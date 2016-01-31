﻿#ifndef EXECUTE_HPP_B0216961_CCAB_46EA_87F4_789AA3A18A43
#define EXECUTE_HPP_B0216961_CCAB_46EA_87F4_789AA3A18A43
#pragma once

/*
execute.hpp

"Запускатель" программ.
*/
/*
Copyright © 1996 Eugene Roshal
Copyright © 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

bool GetShellType(const string&Ext, string &strType, ASSOCIATIONTYPE aType=AT_FILEEXTENSION);

void OpenFolderInShell(const string& Folder);

bool Execute(struct execute_info& Info, bool FolderRun, bool Silent, const std::function<void()>& ConsoleActivator = nullptr);

bool IsBatchExtType(const string&ExtPtr);

bool ExpandOSAliases(string &strStr);

bool ExtractIfExistCommand(string &strCommandText);

#endif // EXECUTE_HPP_B0216961_CCAB_46EA_87F4_789AA3A18A43
