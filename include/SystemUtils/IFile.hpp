#ifndef SYSTEM_UTILS_I_FILE_HPP
#define SYSTEM_UTILS_I_FILE_HPP
/**
 * @file IFile.hpp
 * 
 * This module declares the SystemUtils::IFile interface.
 * 
 * © 2024 by Hatem Nabli.
*/

#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <memory>

namespace SystemUtils {

    /**
     * this is the interface to an object holding a mutable 
     * file byte array and a movable pointer into it.
    */
    class IFile {
    

    public:
        typedef sts::vector< uint8_t > Buffer;
        
        //Methods
    public:
        virtual ~IFile() {}

        /**
         * This method returns the size of the file in bytes.
         * 
         * @return
         *      returns the size of the file in bytes
         */
        virtual uint64_t GetSize() const = 0;

        /**
         * This method extends or truncates the file so 
         * its size becomes the given number of bytes
         * 
         * @param[in] size
         *      This is the given file size in bytes.
         * @return
         *      Returns an andication of whether or not the method
         *      succeeded the size modifcation.
         */
        virtual bool SetSize(uint64_t size) = 0;

        /**
         * This method returns the curent position in the file
         * in bytes.
         * 
         * @return
         *      returns the current position in the file in bytes.
         * 
         */
        virtual uint64_t GetPosition() const = 0;

        /**
         * This method sets the given position in the file in bytes.
         * 
         * @param[in] position
         *      This is the position in bytes to set in the file.
         * 
         */
        virtual void SetPosition(uint64_t position) = 0;

        /**
         * This method reads a region of the file without advancing
         * the current position in the file.
         * 
         * @param[in, out] buffer
         *      This will be modified to contain bytes read from 
         *      the file.
         * @param[in] numBytes
         *      This is the number of bytes to read from the file.
         * @param[in] offset
         *      This is the byte offset in the buffer where to store the
         *      first byte read from the file.
         * 
         * @return
         *      The number of bytes actually read is returned.
         * 
         */
        virtual size_t Peek(Buffer& buffer, size_t numBytes = 0, size_t offset = 0) const = 0;


        /**
         * This method reads a region of the file without advancing
         * the current position in the file.
         * 
         * @param[out] buffer
         *      This Is where to put the bytes tooked from the file
         * @param[in] numBytes
         *      This is the number of bytes to read from the file.
         * 
         * @return
         *      The number of bytes actually read is returned.
         * 
         */
        virtual size_t Peek(void* buffer, size_t numBytes) const = 0;

        /**
         * This method reads a region of the file and advences the
         * current position in the file to be at the byte after the 
         * last byte read
         * 
         * @param[in, out] buffer
         *      This will be modified to contain bytes read from the file.
         * 
         * @param[in] numBytes
         *      This is the number of bytes to read from the file.
         * 
         * @param[in] offset
         *      This is the byte offset in the buffer where to store the
         *      first byte read from the file.
         * @return
         *      The number of bytes actually read is returned.
         * 
        */
       virtual size_t Read(Buffer& buffer, size_t numBytes = 0, size_t offset = 0) = 0;

       /**
        * This method reads a region of the file and advances the
        * current position in the file to be at the byte after 
        * the last byte rad.
        * 
        * @param[out] buffer
        *       This is where to put the bytes read from the file.
        * @param[in] numBytes
        *       This is the number of bytes to read from the file.
        * 
        * @return
        *       The number of bytes actually read is returned.
       */
       virtual size_t Read(void* buffer, size_t numBytes) = 0;


       /**
        * This method writes a region of the file and advances the 
        * current position in the file to be at the bytes after 
        * the last byte written.
        * 
        * @param[in] buffer
        *       This is contain the bytes to write to the file.
        * 
        * @param[in] numBytes
        *       This is the number of bytes to write to the file. 
        * @param[in] offset
        *       This is the byte offset in the buffer where to 
        *       fetch the first byte to write to the file.
        * 
        * @return 
        *       The number of bytes actually written is returned.
        */  
       virtual size_t Write(const Buffer& buffer, size_t numBytes = 0, size_t offset = 0) = 0;

       /**
        * This method write a region of the file and advances the 
        * current position in the file to be at the byte after the 
        * last byte written
        * 
        * @param[in] buffer
        *       This is where to fetch the bytes to write to the file.
        * @param[in] numBytes
        *       This is the number of bytes to write to the file.
        * @return
        *       The number of bytes actually written is returned.
        */
       virtual size_t Write(const void* buffer, size_t numBytes) = 0;

       /**
        * This method creates a new file object which operates on
        * the same file but has its own current file position.
        * 
        * @return
        *       The newly created file object is returned.
        * @retval nullptr
        *   This will be returned if the file could not be cloned.
        *   
        */
       virtual std::shared_ptr< IFile > Clone() = 0;
    };
}

#endif /*SYSTEM_UTILS_I_FILE_HPP*/