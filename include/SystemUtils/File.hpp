#ifndef SYSTEM_UTILS_FILE_HPP
#define SYSTEM_UTILS_FILE_HPP

#include <IFileSystemEntry.hpp>

namespace SystemUtils {

    /**
     * This class represents a file accessed through the 
     * native operating system.
    */
    class File: public IFileSystemEntry {

    public:
        ~File() noexcept;
        File(const File&) = delete;
        File(File&& other) noexcept;
        File& operator=(const File&) = delete;
        File& operator=(File&& other) noexcept;

     // IFileSystemEntry
    public:
        virtual bool IsExisting() override;
        virtual bool IsDirectory() override;
        virtual bool OpenReadOnly() override;
        virtual void Close() override;
        virtual bool OpenReadWrite() override;
        virtual void Destroy() override;
        virtual bool Move(const std::string& newPath) override;
        virtual bool Copy(const std::string& destination) override;
        virtual time_t GetLastModifiedTime() const override;
        virtual std::string GetPath() const override;

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
        virtual std::shared_ptr< IFile > Clone() override;

        //Public methods
    public:
        /**
         * This is the constructor of the calss.
         * 
         * @param[in] path
         *      This is the path to the file in the file system.
        */
       File(std::string path);

       /**
        * This fuction determines whether or not the given path
        * string indicates an absolute path in the fileSystme or not.
        * 
        * @param[in] path
        *       This is the path to check.
        * @return   
        *       Returns an andication of whether or not the given path
        *       string indicates an absolute path in the file system.
       */
      static bool IsAbsolutePath(const std::string& path);

        /**
         * This method returns the absolute path of the application's
         * executable image.
         * 
         * @return
         *     the absolute path of the application's executable
         *     image is returned.
        */
       static std::string GetExeImagePath();

        /**
        * This method returns the directory containing the application's
        * executable image.
        * 
        * @return
        *     The directory containing the application's executable image
        *     is returned.
        */
        static std::string GetExeParentPath();

        /**
         * This method returns the path to the application resource file
         * with the given name.
         * 
         * @param[in] name
         *    This is the name of the application resource file to find.
         * @return
         *    The path to the given application resource file is returned.
         */
        static std::string GetResourceFilePath(const std::string& name);

        /**
         * This method returns the path to the user's home directory.
         * 
         * @return
         *      The path to the user's home directory is returned.
        */
       static std::string GetUserHomeDirectory();

       /**
        * This method returns the directory containing the application's
        * local per user configuration files.
        * 
        * @param[in] nameKey
        *     This is the name of the application.
        * @return
        *    The directory containing the application's local 
        *    per user configuration files is returned.           
       */
        static std::string GetLocalPerUserConfigDirectory(const std::string& nameKey);

        /**
         * This method returns the directory containing the user's
         * saved application files.
         * 
         * @param[in] nameKey
         *      This is a short string identifying the application, to use for
         *      purposes such as naming the directory containg the user's
         *      saved application.
         * @return
         *      Returns the directory containing the user' saved application file.
        */
        static std::string GetUserSavedApplicationDirectory(const std::string& nameKey);

        /**
         * This method lists the contents of a directory.
         * 
         * @param[in] directory
         *      This is the directory to list.
         * 
         * @param[out] list
         *      This is where to store the list of directory entries.
        */
       static void ListDirectory(const std::string& directory, std::vector< std::string >& list);

       /**
        * This method creates a directory if it doesn't already exist.
        * 
        * 
        * @param[in] directory
        *       This is the name of the directory to create.
        * @return
        *       A flag indicating whether or not the method succeeded
        *       is returned.
       */
        static bool CreateDirectory(const std::string& directory);

        /**
         * This method deletes a directory and all its contents.
         * 
         * @param[in] directory
         *      This is the directory to delete.
         * 
         * @return
         *      A flag indicating whether or not the method succeeded
         *      is returned.
        */
        static bool DeleteDirectory(const std::string& directory);

        /**
         * This method copies a directory and all its contents.
         * 
         * @param[in] existingDirectory
         *      This is the name of the directory to copy.
         * @param[out] newDirectory
         *      This is the distination path of the new directory.
         * @return
         *      Returns a flag that indicate whether or not the method
         *      succeededa. 
        */
        static bool CopyDirectory(
            const std::string& existingDirectory,
            const std::string& newDirectory
        );

        /**
         * This method returns a list of directories tha are considered the root
         * directories in the filesystem. For example, in Windows
         * the list contains drive letters.
         * 
         * @return
         *      The list of root directories in the file system is returned.
        */
        static std::vector< std::string > GetDirectoryRoots();

        /**
         * This method returns the current working directory of the process.
         * 
         * @return
         *      The current working directory of the process is returned.
        */
        static std::string GetWorkingDirectory();

        /**
         * Change the current working directory of the process.
         * 
         * @param[in] workingDirectory
         *      This is the directory to set as the current working
         *      directory for the process.
        */
        static void setWorkingDirectory(const std::string& workingDirectory);
    
        //Private properties
    private:
        /**
         * This is the type of structure that contains the private
         * properties of the instance. It is defined in the implementation
         * and declared here to ensure that it is scoped inside the class.
        */
        struct Impl;

        /**
         * This is the type of structure that contains the platform-specific
         * pricate properties of the instance. It is defined in the 
         * platform-specific part of the implementation and declared here to
         * ensure that it is scoped inside the class.
        */
        struct Platform;

        /**
         * This is contains the private properties of the instance.
        */
        std::unique_ptr< Impl > impl_;
    };
}


#endif