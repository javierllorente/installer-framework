# -*- coding: utf-8 -*-
#!/usr/bin/env python
#############################################################################
##
## Copyright (C) 2015 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing/
##
## This file is part of the Qt Installer Framework.
##
## $QT_BEGIN_LICENSE:LGPL$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see http://qt.io/terms-conditions. For further
## information use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file. Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## As a special exception, The Qt Company gives you certain additional
## rights. These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
##
## $QT_END_LICENSE$
##
#############################################################################


import files, os, string, platform
from testexception import TestException

if ( platform.system() == "Windows" ):
    import registry

def makeAbsolutePath( path, relativeTo ):
    if os.path.isabs( path ) or relativeTo == None:
        return path
    else:
        return relativeTo + os.sep + path
        

class TestRunner:
    def __init__( self, testDir, basedir, result ):
        self._testDir = testDir
        self._basedir = basedir
        self._result = result
        
    def checkFile( self, name, size=-1, expectedMd5=None ):
        try:
            files.checkFileImpl( name, size, expectedMd5 )
        except TestException as e:
            self._result.addFailed( name, e.value )
    
    def checkFileList( self, path ):
        lineNum = 0
        haveError = False
        with open( path, 'r' ) as f:
            while True:
                line = f.readline()
                lineNum += 1
                if not line:
                    break
                line = string.strip( line )
                if len( line ) == 0:
                    continue
                segments = string.split( line, ';' )
                if len( segments ) == 3:
                    fp = makeAbsolutePath( segments[0], self._basedir )
                    try:
                        fs = int( segments[1] )
                    except ValueError:
                        fs = -1 #TODO handle error
                    femd5 = segments[2]
                    femd5 = string.strip( femd5 )
                    self.checkFile( fp, fs, femd5 )
                else:
                    self._result.addFailed( path + '_' + str( lineNum ), "Could not parse file list entry: " + line )
                    haveError = True
        if not haveError:
            self._result.addPassed( path, "" )
 
    def checkRegistryList( self, path ):
        haveError = False
        lineNum = 0
        with open( path, 'r' ) as f:
            while True:
                lineNum += 1
                line = f.readline()
                if not line:
                    break
                line = string.strip( line )
                if len( line ) == 0:
                    continue
                segments = string.split( line, ';' )
                if len( segments ) == 3:
                    key = segments[0].strip()
                    value = segments[1].strip()
                    expectedData = segments[2].strip()
                    registry.checkKey( key, value, expectedData )
                else:
                    self._result.addFailed( path + '_' + str( lineNum ), "Could not parse registry list entry: " + line )
                    haveError = True
        if not haveError:
              self._result.addPassed( path, "" )

    def run( self ):
        fileLists = files.locateFiles( self._testDir, "*.filelist" )
        for i in fileLists:
            self.checkFileList( i )
            
        if ( platform.system() == "Windows" ):
            registryLists = files.locateFiles( self._testDir, "*.registrylist" )
            for i in registryLists:
                self.checkRegistryList( i )
        
        # run all .py's in testdir
        # execute all filelists
