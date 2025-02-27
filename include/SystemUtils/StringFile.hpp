#ifndef SYSTEM_UTILS_STRING_FILE_HPP
#define SYSTEM_UTILS_STRING_FILE_HPP

/**
 * @file StringFile.hpp
 *
 * This module declares the SystemUtils::StringFile class.
 *
 * © 2024 by Hatem Nabli
 */

#include <SystemUtils/IFile.hpp>

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

namespace SystemUtils
{
    /**
     * This class represents a file stored in a string.
     */

    class StringFile : public IFile
    {
        // Lifecycle management
    public:
        ~StringFile() noexcept;
        StringFile(const StringFile&);
        StringFile(StringFile&&) noexcept;
        StringFile& operator=(const StringFile&);
        StringFile& operator=(StringFile&&) noexcept;

    public:
        /**
         * This is an instance constructor
         *
         * @param[in] initialValue
         *      This is the initial contents of the file.
         */
        StringFile(std::string initialValue = "");

        /**
         * This is an instance constructor
         *
         * @param[in] initialValue
         *  This is the initial content of the file.
         *
         */
        StringFile(std::vector<uint8_t> initialValue);

        /**
         * This is the typecast to std::string operator.
         */
        operator std::string() const;

        /**
         * This is the typecast to std::string operator.
         */
        operator std::vector<uint8_t>() const;

        /**
         * This is the assignment from std::string operator.
         */
        StringFile& operator=(const std::string& b);

        /**
         * This is the assignment from std::string operator.
         */
        StringFile& operator=(const std::vector<uint8_t>& b);

        /**
         * This method removes the given number of bytes from the front
         * of the string, and moves the file pointer back to either
         * the front of the file, or back the same number of bytes removec,
         * whichever is closer.
         *
         * @param[in] numBytes
         *      This is the number of bytes to remove from the front of the file.
         */
        void Remove(size_t numBytes);

        // IFile
    public:
        virtual uint64_t GetSize() const override;
        virtual bool SetSize(uint64_t size) override;
        virtual uint64_t GetPosition() const override;
        virtual void SetPosition(uint64_t position) override;
        virtual size_t Peek(Buffer& buffer, size_t numBytes = 0, size_t offset = 0) const override;
        virtual size_t Peek(void* buffer, size_t numBytes) const override;
        virtual size_t Read(Buffer& buffer, size_t numBytes = 0, size_t offset = 0) override;
        virtual size_t Read(void* buffer, size_t numBytes) override;
        virtual size_t Write(const Buffer& buffer, size_t numBytes = 0, size_t offset = 0) override;
        virtual size_t Write(const void* buffer, size_t numBytes) override;
        virtual std::shared_ptr<IFile> Clone() override;

        // Private properties
    private:
        /**
         * This is the type of structure that contains the private
         * properties of the instance. It is defined in the implementation
         * and declared here to ensure that it is scoped inside the class.
         */
        struct Impl;

        /**
         * This contain the private properties of the instance.
         */
        std::unique_ptr<Impl> impl_;
    };
}  // namespace SystemUtils
#endif /*SYSTEM_UTILS_STRING_FILE_HPP*/