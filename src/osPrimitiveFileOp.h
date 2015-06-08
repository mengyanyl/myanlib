/*******************************************************************************
   Copyright (C) 2013 MyanDB Software Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.
*******************************************************************************/

#ifndef OSPRIMITIVEFILEOP_HPP__
#define OSPRIMITIVEFILEOP_HPP__

namespace myan
{
namespace utils
{
   
#include "core.h"

#ifdef _WINDOWS

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define OSS_F_GETLK        F_GETLK64
#define OSS_F_SETLK        F_SETLK64
#define OSS_F_SETLKW       F_SETLKW64

#define oss_struct_statfs  struct statfs64
#define oss_statfs         statfs64
#define oss_fstatfs        fstatfs64
#define oss_struct_statvfs struct statvfs64
#define oss_statvfs        statvfs64
#define oss_fstatvfs       fstatvfs64
#define oss_struct_stat    struct _stati64
#define oss_struct_flock   struct flock64
#define oss_stat           stat64
#define oss_lstat          lstat64
#define oss_fstat          _fstati64
#define oss_open           _open
#define oss_lseek          _lseeki64
#define oss_ftruncate      ftruncate64
#define oss_off_t          _int64
#define oss_close          _close
#define oss_access         access
#define oss_chmod
#define oss_read           read
#define oss_write          write

#define O_RDWR				_O_RDWR
#define O_RDONLY			_O_RDONLY
#define O_WRONLY			_O_WRONLY
#define O_CREAT				_O_CREAT
#define O_TRUNC				_O_TRUNC

#define OSS_HANDLE			int
#define OSS_INVALID_HANDLE_FD_VALUE (OSS_HANDLE(-1))
#elif defined(__CYGWIN__)
#define OSS_HANDLE		   int
#define OSS_F_GETLK        F_GETLK
#define OSS_F_SETLK        F_SETLK
#define OSS_F_SETLKW       F_SETLKW

#define oss_struct_statfs  struct statfs
#define oss_statfs         statfs
#define oss_fstatfs        fstatfs
#define oss_struct_statvfs struct statvfs
#define oss_statvfs        statvf
#define oss_fstatvfs       fstatvfs
#define oss_struct_stat    struct stat
#define oss_struct_flock   struct flock
#define oss_stat           stat
#define oss_lstat          lstat
#define oss_fstat          fstat
#define oss_open           open
#define oss_lseek          lseek
#define oss_ftruncate      ftruncate
#define oss_off_t          int64_t 
#define oss_close          close
#define oss_access         access
#define oss_chmod          chmod
#define oss_read           read
#define oss_write          write
#define OSS_INVALID_HANDLE_FD_VALUE (-1)

#elif defined(__linux__)
#define OSS_HANDLE		   int
#define OSS_F_GETLK        F_GETLK64
#define OSS_F_SETLK        F_SETLK64
#define OSS_F_SETLKW       F_SETLKW64

#define oss_struct_statfs  struct statfs64
#define oss_statfs         statfs64
#define oss_fstatfs        fstatfs64
#define oss_struct_statvfs struct statvfs64
#define oss_statvfs        statvfs64
#define oss_fstatvfs       fstatvfs64
#define oss_struct_stat    struct stat64
#define oss_struct_flock   struct flock64
#define oss_stat           stat64
#define oss_lstat          lstat64
#define oss_fstat          fstat64
#define oss_open           open64
#define oss_lseek          lseek64
#define oss_ftruncate      ftruncate64
#define oss_off_t          int64_t 
#define oss_close          close
#define oss_access         access
#define oss_chmod          chmod
#define oss_read           read
#define oss_write          write

#define OSS_INVALID_HANDLE_FD_VALUE (-1)

#endif // _WINDOWS

#define OSS_PRIMITIVE_FILE_OP_FWRITE_BUF_SIZE 2048
#define OSS_PRIMITIVE_FILE_OP_READ_ONLY     (((unsigned int)1) << 1)
#define OSS_PRIMITIVE_FILE_OP_WRITE_ONLY    (((unsigned int)1) << 2)
#define OSS_PRIMITIVE_FILE_OP_OPEN_EXISTING (((unsigned int)1) << 3)
#define OSS_PRIMITIVE_FILE_OP_OPEN_ALWAYS   (((unsigned int)1) << 4)
#define OSS_PRIMITIVE_FILE_OP_OPEN_TRUNC    (((unsigned int)1) << 5)

typedef oss_off_t offsetType ;

class osPrimitiveFileOp
{
public :
   typedef  OSS_HANDLE    handleType ;
private :
   handleType _fileHandle ;
   osPrimitiveFileOp( const osPrimitiveFileOp & ) {}
   const osPrimitiveFileOp &operator=( const osPrimitiveFileOp & ) ;
   bool _bIsStdout ;

protected :
   void setFileHandle( handleType handle ) ;

public :
   osPrimitiveFileOp() ;
   int Open
   (
      const char * pFilePath,
      unsigned int options = OSS_PRIMITIVE_FILE_OP_OPEN_ALWAYS
   ) ;
   void openStdout() ;
   void Close() ;
   bool isValid( void ) ;
   int Read( const size_t size, void * const pBuf, int * const pBytesRead ) ;

   int Write( const void * pBuf, size_t len = 0 ) ;

   int fWrite( const char * fmt, ... ) ;

   offsetType getCurrentOffset (void) const ;

   void seekToOffset( offsetType offset ) ;

   void seekToEnd( void ) ;

   int getSize( offsetType * const pFileSize ) ;

   handleType getHandle( void ) const
   {
      return _fileHandle ;
   }
} ;

}
}
#endif
