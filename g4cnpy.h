//Copyright (C) 2011  Carl Rogers
//Released under MIT License
//license available in LICENSE file, or at http://www.opensource.org/licenses/mit-license.php

// Modified by Lajos Palanki<lala5th@gmail.com> 

#ifndef LIBG4CNPY_H_
#define LIBG4CNPY_H_

#include<string>
#include<stdexcept>
#include<sstream>
#include<vector>
#include<cstdio>
#include<typeinfo>
#include<iostream>
#include<cassert>
#include<zlib.h>
#include<map>
#include<memory>
#include<cstdint>
#include<numeric>
#include<cstring>

#ifdef WIN32
#ifdef G4NPYANALYSIS_EXPORT
    #define WINAPI __declspec(dllexport)
#else
    #define WINAPI __declspec(dllimport)
#endif
#else
    #define WINAPI
#endif

namespace g4cnpy {

    struct NpyArray {
        NpyArray(const std::vector<size_t>& _shape, size_t _word_size, bool _fortran_order) :
            shape(_shape), word_size(_word_size), fortran_order(_fortran_order)
        {
            num_vals = 1;
            for(size_t i = 0;i < shape.size();i++) num_vals *= shape[i];
            data_holder = std::shared_ptr<std::vector<char>>(
                new std::vector<char>(num_vals * word_size));
        }

        NpyArray() : shape(0), word_size(0), fortran_order(0), num_vals(0) { }

        template<typename T>
        T* data() {
            if ((((uintptr_t)data_holder->data()) % alignof(T) != 0)){
                assert(word_size == sizeof(T));
                T* data_container = new T[num_vals];
                std::memcpy(data_container,data_holder->data(), num_vals*word_size);
                data_holder.reset(new std::vector<char>((char*) data_container, (char*) (data_container + num_vals)));
                return data_container;
            }
            return reinterpret_cast<T*>(&(*data_holder)[0]);
        }

        template<typename T>
        const T* data() const {
            assert(((uintptr_t)data_holder->data()) % alignof(T) == 0);
            return reinterpret_cast<T*>(&(*data_holder)[0]);
        }

        template<typename T>
        std::vector<T> as_vec() const {
            const T* p = data<T>();
            return std::vector<T>(p, p+num_vals);
        }

        size_t num_bytes() const {
            return data_holder->size();
        }

        std::shared_ptr<std::vector<char>> data_holder;
        std::vector<size_t> shape;
        size_t word_size;
        bool fortran_order;
        size_t num_vals;
    };

    using npz_t = std::map<std::string, NpyArray>;

    WINAPI char BigEndianTest();
    WINAPI char map_type(const std::type_info& t);
    template<typename T> std::vector<char> create_npy_header(const std::vector<size_t>& shape);
    template<typename... COLS> std::vector<char> create_npy_tuple_header(const std::vector<size_t>& shape);
    WINAPI void parse_npy_header(FILE* fp,size_t& word_size, std::vector<size_t>& shape, bool& fortran_order);
    WINAPI void parse_npy_header(unsigned char* buffer,size_t& word_size, std::vector<size_t>& shape, bool& fortran_order);
    WINAPI void parse_npy_header(FILE* fp,std::vector<char> dtype_descr, std::vector<size_t>& shape, bool& fortran_order);
    WINAPI void parse_zip_footer(FILE* fp, uint16_t& nrecs, size_t& global_header_size, size_t& global_header_offset);
    WINAPI npz_t npz_load(std::string fname);
    WINAPI NpyArray npz_load(std::string fname, std::string varname);
    WINAPI NpyArray npy_load(std::string fname);

    template<typename T> std::vector<char>& operator+=(std::vector<char>& lhs, const T rhs) {
        //write in little endian
        for(size_t byte = 0; byte < sizeof(T); byte++) {
            char val = *((char*)&rhs+byte);
            lhs.push_back(val);
        }
        return lhs;
    }

    template<> WINAPI std::vector<char>& operator+=(std::vector<char>& lhs, const std::string rhs);
    template<> WINAPI std::vector<char>& operator+=(std::vector<char>& lhs, const char* rhs);


    template<size_t N, typename... COLS> constexpr void iterate_dtype(std::vector<char>& descr){

        char endianTest = BigEndianTest();

        descr += "('";
        descr += std::to_string(N);
        descr += "', '";
        descr += endianTest;
        descr += map_type(typeid(typename std::tuple_element<N,std::tuple<COLS...>>::type));
        descr += std::to_string(sizeof(typename std::tuple_element<N,std::tuple<COLS...>>::type));
        descr += "'";
        if constexpr(N != sizeof...(COLS)-1){
            descr += "),";
            iterate_dtype<N+1,COLS...>(descr);
        }else{
            descr += ")";
        }

    }

    template<typename... COLS> void construct_tuple_dtype(std::vector<char>& descr){

        descr += "[";
        iterate_dtype<0,COLS...>(descr);
        descr += "]";

    }

    template<size_t N,typename... COLS> constexpr void iterative_write_data(std::tuple<COLS...> data, const std::vector<size_t>& shape,FILE* fp){

        fwrite(&std::get<N>(data),sizeof(typename std::tuple_element<N,std::tuple<COLS...>>::type),1,fp);
        if constexpr(N != sizeof...(COLS) - 1){
            iterative_write_data<N+1,COLS...>(data, shape, fp);
        }
    }

    template<size_t N,typename... COLS> constexpr size_t size_of(){

        size_t size = 0;
        size += sizeof(typename std::tuple_element<N,std::tuple<COLS...>>::type);
        if constexpr(N != sizeof...(COLS) - 1){
            size += size_of<N+1,COLS...>();
        }
        return size;
    }

    template<size_t N,typename... COLS> constexpr void generate_crc(uint32_t& crc, std::tuple<COLS...> data){

        crc = crc32(crc,(unsigned char*)&std::get<N>(data),sizeof(typename std::tuple_element<N,std::tuple<COLS...>>::type));
        if constexpr(N != sizeof...(COLS) - 1){
            generate_crc<N+1,COLS...>(crc,data);
        }
    }

    template<typename T> void npy_save(std::string fname, const T* data, const std::vector<size_t> shape, std::string mode = "w") {
        FILE* fp = NULL;
        std::vector<size_t> true_data_shape; //if appending, the shape of existing + new data

        if(mode == "a") fp = fopen(fname.c_str(),"r+b");

        if(fp) {
            //file exists. we need to append to it. read the header, modify the array size
            size_t word_size;
            bool fortran_order;
            parse_npy_header(fp,word_size,true_data_shape,fortran_order);
            assert(!fortran_order);

            if(word_size != sizeof(T)) {
                std::cout<<"libnpy error: "<<fname<<" has word size "<<word_size<<" but npy_save appending data sized "<<sizeof(T)<<"\n";
                assert( word_size == sizeof(T) );
            }
            if(true_data_shape.size() != shape.size()) {
                std::cout<<"libnpy error: npy_save attempting to append misdimensioned data to "<<fname<<"\n";
                assert(true_data_shape.size() != shape.size());
            }

            for(size_t i = 1; i < shape.size(); i++) {
                if(shape[i] != true_data_shape[i]) {
                    std::cout<<"libnpy error: npy_save attempting to append misshaped data to "<<fname<<"\n";
                    assert(shape[i] == true_data_shape[i]);
                }
            }
            true_data_shape[0] += shape[0];
        }
        else {
            fp = fopen(fname.c_str(),"wb");
            true_data_shape = shape;
        }

        std::vector<char> header = create_npy_header<T>(true_data_shape);
        size_t nels = std::accumulate(shape.begin(),shape.end(),1,std::multiplies<size_t>());

        fseek(fp,0,SEEK_SET);
        fwrite(&header[0],sizeof(char),header.size(),fp);
        fseek(fp,0,SEEK_END);
        fwrite(data,sizeof(T),nels,fp);
        fclose(fp);
    }

    template<typename... COLS> void npy_save(std::string fname, const std::tuple<COLS...>* data, const std::vector<size_t> shape, std::string mode = "w") {
        FILE* fp = NULL;
        std::vector<size_t> true_data_shape; //if appending, the shape of existing + new data

        if(mode == "a") fp = fopen(fname.c_str(),"r+b");

        if(fp) {
            //file exists. we need to append to it. read the header, modify the array size
            bool fortran_order;
            std::vector<char> dtype;
            construct_tuple_dtype<COLS...>(dtype);
            parse_npy_header(fp,dtype,true_data_shape,fortran_order);
            assert(!fortran_order);

            if(true_data_shape.size() != shape.size()) {
                std::cout<<"libnpy error: npy_save attempting to append misdimensioned data to "<<fname<<"\n";
                assert(true_data_shape.size() != shape.size());
            }

            for(size_t i = 1; i < shape.size(); i++) {
                if(shape[i] != true_data_shape[i]) {
                    std::cout<<"libnpy error: npy_save attempting to append misshaped data to "<<fname<<"\n";
                    assert(shape[i] == true_data_shape[i]);
                }
            }
            true_data_shape[0] += shape[0];
        }
        else {
            fp = fopen(fname.c_str(),"wb");
            true_data_shape = shape;
        }

        std::vector<char> header = create_npy_tuple_header<COLS...>(true_data_shape);

        fseek(fp,0,SEEK_SET);
        fwrite(&header[0],sizeof(char),header.size(),fp);
        fseek(fp,0,SEEK_END);
        size_t len = 1;
        if(shape.empty()){
            len = 0;
        }
        for(auto d : shape){
            len *= d;
        }
        for(size_t i = 0;i < len;i++){
            iterative_write_data<0>(data[i],shape,fp);
        }
        fclose(fp);
    }

    template<typename T> void npz_save(std::string zipname, std::string fname, const T* data, const std::vector<size_t>& shape, std::string mode = "w")
    {
        //first, append a .npy to the fname
        fname += ".npy";

        //now, on with the show
        FILE* fp = NULL;
        uint16_t nrecs = 0;
        size_t global_header_offset = 0;
        std::vector<char> global_header;

        if(mode == "a") fp = fopen(zipname.c_str(),"r+b");

        if(fp) {
            //zip file exists. we need to add a new npy file to it.
            //first read the footer. this gives us the offset and size of the global header
            //then read and store the global header.
            //below, we will write the the new data at the start of the global header then append the global header and footer below it
            size_t global_header_size;
            parse_zip_footer(fp,nrecs,global_header_size,global_header_offset);
            fseek(fp,global_header_offset,SEEK_SET);
            global_header.resize(global_header_size);
            size_t res = fread(&global_header[0],sizeof(char),global_header_size,fp);
            if(res != global_header_size){
                throw std::runtime_error("npz_save: header read error while adding to existing zip");
            }
            fseek(fp,global_header_offset,SEEK_SET);
        }
        else {
            fp = fopen(zipname.c_str(),"wb");
        }

        std::vector<char> npy_header = create_npy_header<T>(shape);

        size_t nels = std::accumulate(shape.begin(),shape.end(),1,std::multiplies<size_t>());
        size_t nbytes = nels*sizeof(T) + npy_header.size();

        //get the CRC of the data to be added
        uint32_t crc = crc32(0L,(unsigned char*)&npy_header[0],npy_header.size());
        crc = crc32(crc,(unsigned char*)data,nels*sizeof(T));

        //build the local header
        std::vector<char> local_header;
        local_header += "PK"; //first part of sig
        local_header += (uint16_t) 0x0403; //second part of sig
        local_header += (uint16_t) 20; //min version to extract
        local_header += (uint16_t) 0; //general purpose bit flag
        local_header += (uint16_t) 0; //compression method
        local_header += (uint16_t) 0; //file last mod time
        local_header += (uint16_t) 0;     //file last mod date
        local_header += (uint32_t) crc; //crc
        local_header += (uint32_t) nbytes; //compressed size
        local_header += (uint32_t) nbytes; //uncompressed size
        local_header += (uint16_t) fname.size(); //fname length
        local_header += (uint16_t) 0; //extra field length
        local_header += fname;

        //build global header
        global_header += "PK"; //first part of sig
        global_header += (uint16_t) 0x0201; //second part of sig
        global_header += (uint16_t) 20; //version made by
        global_header.insert(global_header.end(),local_header.begin()+4,local_header.begin()+30);
        global_header += (uint16_t) 0; //file comment length
        global_header += (uint16_t) 0; //disk number where file starts
        global_header += (uint16_t) 0; //internal file attributes
        global_header += (uint32_t) 0; //external file attributes
        global_header += (uint32_t) global_header_offset; //relative offset of local file header, since it begins where the global header used to begin
        global_header += fname;

        //build footer
        std::vector<char> footer;
        footer += "PK"; //first part of sig
        footer += (uint16_t) 0x0605; //second part of sig
        footer += (uint16_t) 0; //number of this disk
        footer += (uint16_t) 0; //disk where footer starts
        footer += (uint16_t) (nrecs+1); //number of records on this disk
        footer += (uint16_t) (nrecs+1); //total number of records
        footer += (uint32_t) global_header.size(); //nbytes of global headers
        footer += (uint32_t) (global_header_offset + nbytes + local_header.size()); //offset of start of global headers, since global header now starts after newly written array
        footer += (uint16_t) 0; //zip file comment length

        //write everything
        fwrite(&local_header[0],sizeof(char),local_header.size(),fp);
        fwrite(&npy_header[0],sizeof(char),npy_header.size(),fp);
        fwrite(data,sizeof(T),nels,fp);
        fwrite(&global_header[0],sizeof(char),global_header.size(),fp);
        fwrite(&footer[0],sizeof(char),footer.size(),fp);
        fclose(fp);
    }

    template<typename... COLS> void npz_save(std::string zipname, std::string fname, const std::tuple<COLS...>* data, const std::vector<size_t>& shape, std::string mode = "w")
    {
        //first, append a .npy to the fname
        fname += ".npy";

        //now, on with the show
        FILE* fp = NULL;
        uint16_t nrecs = 0;
        size_t global_header_offset = 0;
        std::vector<char> global_header;

        if(mode == "a") fp = fopen(zipname.c_str(),"r+b");

        if(fp) {
            //zip file exists. we need to add a new npy file to it.
            //first read the footer. this gives us the offset and size of the global header
            //then read and store the global header.
            //below, we will write the the new data at the start of the global header then append the global header and footer below it
            size_t global_header_size;
            parse_zip_footer(fp,nrecs,global_header_size,global_header_offset);
            fseek(fp,global_header_offset,SEEK_SET);
            global_header.resize(global_header_size);
            size_t res = fread(&global_header[0],sizeof(char),global_header_size,fp);
            if(res != global_header_size){
                throw std::runtime_error("npz_save: header read error while adding to existing zip");
            }
            fseek(fp,global_header_offset,SEEK_SET);
        }
        else {
            fp = fopen(zipname.c_str(),"wb");
        }
        std::vector<char> npy_header = create_npy_tuple_header<COLS...>(shape);

        size_t nels = std::accumulate(shape.begin(),shape.end(),1,std::multiplies<size_t>());
        size_t nbytes = nels*size_of<0,COLS...>() + npy_header.size();

        //get the CRC of the data to be added
        size_t len = 1;
        if(shape.empty()){
            len = 0;
        }
        for(auto d : shape){
            len *= d;
        }
        uint32_t crc = crc32(0L,(unsigned char*)&npy_header[0],npy_header.size());
        for(size_t i = 0;i < len;i++)
            generate_crc<0,COLS...>(crc,data[i]);

        //build the local header
        std::vector<char> local_header;
        local_header += "PK"; //first part of sig
        local_header += (uint16_t) 0x0403; //second part of sig
        local_header += (uint16_t) 20; //min version to extract
        local_header += (uint16_t) 0; //general purpose bit flag
        local_header += (uint16_t) 0; //compression method
        local_header += (uint16_t) 0; //file last mod time
        local_header += (uint16_t) 0;     //file last mod date
        local_header += (uint32_t) crc; //crc
        local_header += (uint32_t) nbytes; //compressed size
        local_header += (uint32_t) nbytes; //uncompressed size
        local_header += (uint16_t) fname.size(); //fname length
        local_header += (uint16_t) 0; //extra field length
        local_header += fname;

        //build global header
        global_header += "PK"; //first part of sig
        global_header += (uint16_t) 0x0201; //second part of sig
        global_header += (uint16_t) 20; //version made by
        global_header.insert(global_header.end(),local_header.begin()+4,local_header.begin()+30);
        global_header += (uint16_t) 0; //file comment length
        global_header += (uint16_t) 0; //disk number where file starts
        global_header += (uint16_t) 0; //internal file attributes
        global_header += (uint32_t) 0; //external file attributes
        global_header += (uint32_t) global_header_offset; //relative offset of local file header, since it begins where the global header used to begin
        global_header += fname;

        //build footer
        std::vector<char> footer;
        footer += "PK"; //first part of sig
        footer += (uint16_t) 0x0605; //second part of sig
        footer += (uint16_t) 0; //number of this disk
        footer += (uint16_t) 0; //disk where footer starts
        footer += (uint16_t) (nrecs+1); //number of records on this disk
        footer += (uint16_t) (nrecs+1); //total number of records
        footer += (uint32_t) global_header.size(); //nbytes of global headers
        footer += (uint32_t) (global_header_offset + nbytes + local_header.size()); //offset of start of global headers, since global header now starts after newly written array
        footer += (uint16_t) 0; //zip file comment length

        //write everything
        fwrite(&local_header[0],sizeof(char),local_header.size(),fp);
        fwrite(&npy_header[0],sizeof(char),npy_header.size(),fp);
        for(size_t i = 0;i < len;i++){
            iterative_write_data<0>(data[i],shape,fp);
        }
        fwrite(&global_header[0],sizeof(char),global_header.size(),fp);
        fwrite(&footer[0],sizeof(char),footer.size(),fp);
        fclose(fp);
    }

    template<typename T> void npy_save(std::string fname, const std::vector<T> data, std::string mode = "w") {
        std::vector<size_t> shape;
        shape.push_back(data.size());
        npy_save(fname, &data[0], shape, mode);
    }

    template<typename... COLS> void npy_save(std::string fname, const std::vector<std::tuple<COLS...>> data, std::string mode = "w") {
        std::vector<size_t> shape;
        shape.push_back(data.size());
        npy_save(fname, &data[0], shape, mode);
    }

    template<typename T> void npz_save(std::string zipname, std::string fname, const std::vector<T> data, std::string mode = "w") {
        std::vector<size_t> shape;
        shape.push_back(data.size());
        npz_save(zipname, fname, &data[0], shape, mode);
    }

    template<typename... COLS> void npz_save(std::string zipname, std::string fname, const std::vector<std::tuple<COLS...>> data, std::string mode = "w") {
        std::vector<size_t> shape;
        shape.push_back(data.size());
        npz_save<COLS...>(zipname, fname, &data[0], shape, mode);
    }

    template<typename... COLS> std::vector<char> create_npy_tuple_header(const std::vector<size_t>& shape) {

        std::vector<char> dict;
        dict += "{'descr': ";
        construct_tuple_dtype<COLS...>(dict);
        dict += ", 'fortran_order': False, 'shape': (";
        std::string length;
        size_t shape_length = 0;
        for(size_t i = 0;i < shape.size();i++) {
            length = std::to_string(shape[i]);
            dict += length;
            dict += ", ";
            shape_length += length.size() + 1;
        }

        dict += "), }";

        // Add padding so that impossible to overflow [! Otherwise after much data is written the padding might overwrite data]
        // Division by 3.32 is because it is each base 10 digit is represented as a byte
        dict.insert(dict.end(), sizeof(size_t)*shape.size()*8/3.32 - length.size(), ' ');

        //pad with spaces so that preamble+dict is modulo 16 bytes. preamble is 10 bytes. dict needs to end with \n
        int remainder = 16 - (10 + dict.size()) % 16;
        dict.insert(dict.end(),remainder,' ');
        dict.back() = '\n';

        std::vector<char> header;
        header += (char) 0x93;
        header += "NUMPY";
        header += (char) 0x01; //major version of numpy format
        header += (char) 0x00; //minor version of numpy format
        header += (uint16_t) dict.size();
        header.insert(header.end(),dict.begin(),dict.end());

        return header;
    }

    template<typename T> std::vector<char> create_npy_header(const std::vector<size_t>& shape) {

        std::vector<char> dict;
        dict += "{'descr': '";
        dict += BigEndianTest();
        dict += map_type(typeid(T));
        dict += std::to_string(sizeof(T));
        dict += "', 'fortran_order': False, 'shape': (";
        std::string length;
        size_t shape_length = 0;
        for(size_t i = 0;i < shape.size();i++) {
            length = std::to_string(shape[i]);
            dict += length;
            dict += ", ";
            shape_length += length.size() + 1;
        }

        dict += "), }";

        // Add padding so that impossible to overflow [! Otherwise after much data is written the padding might overwrite data]
        // Division by 3.32 is because it is each base 10 digit is represented as a byte
        dict.insert(dict.end(), sizeof(size_t)*shape.size()*8/3.32 - shape_length, ' ');

        //pad with spaces so that preamble+dict is modulo 16 bytes. preamble is 10 bytes. dict needs to end with \n
        int remainder = 16 - (10 + dict.size()) % 16;
        dict.insert(dict.end(),remainder,' ');
        dict.back() = '\n';

        std::vector<char> header;
        header += (char) 0x93;
        header += "NUMPY";
        header += (char) 0x01; //major version of numpy format
        header += (char) 0x00; //minor version of numpy format
        header += (uint16_t) dict.size();
        header.insert(header.end(),dict.begin(),dict.end());

        return header;
    }


}

#undef WINAPI

#endif
