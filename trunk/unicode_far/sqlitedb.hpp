#pragma once

/*
sqlitedb.hpp

������ sqlite api ��� c++.
*/
/*
Copyright � 2011 Far Group
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

namespace sqlite
{
	struct sqlite3;
	struct sqlite3_stmt;
}

class SQLiteStmt
{
	int param;

protected:
	sqlite::sqlite3_stmt* pStmt;

public:
	enum ColumnType
	{
		TYPE_INTEGER,
		TYPE_STRING,
		TYPE_BLOB,
		TYPE_UNKNOWN
	};

	SQLiteStmt();
	~SQLiteStmt();
	bool Finalize();
	SQLiteStmt& Reset();
	bool Step();
	bool StepAndReset();
	SQLiteStmt& Bind(int Value);
	SQLiteStmt& Bind(unsigned int Value) {return Bind(static_cast<int>(Value));}
	SQLiteStmt& Bind(__int64 Value);
	SQLiteStmt& Bind(unsigned __int64 Value) {return Bind(static_cast<__int64>(Value));}
	SQLiteStmt& Bind(const string& Value, bool bStatic=true);
	SQLiteStmt& Bind(const void *Value, size_t Size, bool bStatic=true);
	const wchar_t *GetColText(int Col);
	const char *GetColTextUTF8(int Col);
	int GetColBytes(int Col);
	int GetColInt(int Col);
	unsigned __int64 GetColInt64(int Col);
	const char *GetColBlob(int Col);
	ColumnType GetColType(int Col);
	friend class SQLiteDb;
};

class SQLiteDb {
	sqlite::sqlite3 *pDb;
	int init_status;
	int db_exists;

protected:
	string strPath;
	string strName;

public:
	SQLiteDb();
	virtual ~SQLiteDb();
	bool Open(const string& DbFile, bool Local, bool WAL=false);
	void Initialize(const string& DbName, bool Local = false);
	bool Exec(const char *Command);
	bool BeginTransaction();
	bool EndTransaction();
	bool RollbackTransaction();
	bool IsOpen();
	bool InitStmt(SQLiteStmt &stmtStmt, const wchar_t *Stmt);
	int Changes();
	unsigned __int64 LastInsertRowID();
	bool Close();
	bool SetWALJournalingMode();
	bool EnableForeignKeysConstraints();
	virtual bool InitializeImpl(const string& DbName, bool Local) = 0;
	int InitStatus(string& name, bool full_name);
	bool IsNew() { return db_exists <= 0; }
};
