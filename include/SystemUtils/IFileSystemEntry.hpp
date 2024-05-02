#ifndef SYSTEM_UTILS_I_FILE_SYSTEM_ENTRY_HPP
#define SYSTEM_UTILS_I_FILE_SYSTEM_ENTRY_HPP
#pragma once

/**
 * @file IFileSystemEntry.hpp
 * This module declares an abstract interface for an object representing
 * an entry in a file system.
 * 
 * © 2024 by Hatem Nabli
*/

#include "IFile.hpp"

#include <string>
#include <time.h>

namespace SystemUtils {

    /**
     * This class represents an entry in a file system.
    */
    class IFileSystemEntry: public IFile {
    
    public:
        /**
         * This method is used to check if the file exists in
         * the file system.
         * 
         * @return
         *      A flag is returned that indicates whather or not the
         *      file exists in the file system.
         */
        virtual bool IsExisting() = 0;

        /**
         * This method is used to check if the file is a directory.
         * 
         * @return
         *      returns a flag that indicat whether or not the file
         *       exists in the file system as a directory.
         */
        virtual bool IsDirectory() = 0;

        /**
         * This method opens the file for reading, expecting it 
         * to already exist.
         * 
         * @return
         *      returns a flag that indicat whether or not the method 
         *      succeeded.
         */
        virtual bool OpenReadOnly() = 0;

        /**
         * This method closes the fle, applying any changes made to it.
         */
        virtual void Close() = 0;

        /**
         * This method opens the file for reading and writing, creating
         * it if it does not already exist.
         * 
         * @return
         *      A flag indicating whether or not the method succeeded
         *      is returned.
        */
        virtual bool OpenReadWrite() = 0;

        /**
         * This method destroys the file in the file system.
        */
        virtual void Destroy() = 0;

        /**
         * This method moves the file to a new path in the file system.
         * 
         * @param[in] newPath
         *      This si the new path to which to move the file.
         * @return
         *      returns a flag that indicate whether or not the
         *      method succeded.
        */
        virtual bool Move(const std::string& newPath) = 0;

        /**
         * This method copies the file to another location in 
         * the file system.
         * 
         * @param[in] destination
         *      This is the file name and path to ctreate as 
         *      a copy of the file.
         * @return
         *      Returns an indication of whether or not the 
         *      method succeeded.
        */
       virtual bool Copy(const std::string& destination) = 0;

       /**
        * This method returns the last time whene the file was 
        * modified.
        * 
        * @return
        *       The time the file was modified is returned.
        * 
        */ 
       virtual time_t GetLastModifiedTime() const = 0;

       /**
        * This method returns the path of the file.
        * 
        * @return
        *       The path of the file is returned.
       */
      virtual std::string GetPath() const = 0;
    };  
}

#endif /*SYSTEM_UTILS_I_FILE_SYSTEM_ENTRY_HPP*/