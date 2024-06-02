#ifndef SYSTEM_UTILS_DYNAMIC_LIBRARY_HPP
#define SYSTEM_UTILS_DYNAMIC_LIBRARY_HPP

/**
 * @file DynamicLibrary.hpp
 * 
 * This module declares the SystemUtils::DynamicLibrary class.
 * 
 * © 2024 by Hatem Nabli
*/

#include <memory>
#include <string>

namespace SystemUtils {

    /**
     * This class represents a dynamically loaded library.
    */
   class DynamicLibrary {
        // Lifecycle managment
    public:
        ~DynamicLibrary();
        DynamicLibrary(const DynamicLibrary&) = delete;
        DynamicLibrary(DynamicLibrary&& other) noexcept;
        DynamicLibrary& operator=(const DynamicLibrary& other) = delete;
        DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;

    public:
        /**
         * This is an instance constructor. 
        */ 
        DynamicLibrary();

        /**
        * This method load the dynamic library from storage,
        * linking it into the running program.
        * 
        * @param[in] path
        *       This is the path to the directory containing
        *       the dinamic library.
        * 
        * @param[in] name
        *       This is the name of the dynamic library, without
        *       any prefix, suffix, or file extension.
        * 
        * @return
        *       An indication of whether or not the library was successfully
        * loaded and linked to the program is returned.
        * 
        */
        bool Load(const std::string& path, const std::string& name);

        /**
         * This method unlinks the dynamic library from the running program.
         * 
         * @note
         *    Do not call this method until all objects and recources
         *    (i.e. threads) from the library have been destroyes,
         * otherwise the program may crash.
         */
        void Unload();

        /**
         * This method locates the procedure (function) that has the given
         * name in the library, and returns its address.
         * 
         * @param[in] name
         *      This is the name of the function in the loaded library.
         * 
         * @return
         *      The addres of the given function in the library is returned.
         * 
         * @retval nullptr
         *      This is returned if the loader could not find any function
         *      with the given name in the library.
         * @note
         *      This method should only be called while the library is loaded.
        */
        void* GetProcedure(const std::string& name);

        /**
         * This method returns a human-readable string describing
         * the last error that occurred calling another
         * method on the object.
         * 
         * @return
         *      A human-readable string describing the last error 
         *      that occured calling another method of the
         *      object is returned.
        */
        std::string GetLastError();

        // properties
    private:

        struct Impl;
        
        
        std::unique_ptr< Impl > impl_;
     
   };
}

#endif