// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __TEXT_UTILITIES_H__
#define __TEXT_UTILITIES_H__

/*
============
sdTextUtilities
============
*/
class idTextUtilities {
public:
	int		Write( idFile*, const char* string, bool indent = true );
	void	Indent( idFile* );
	void	Unindent( idFile* );

	void	CloseFile( idFile* );

private:
	idList< idFile* > fileList;
	idList< int > indentList;
};


typedef idSingleton< idTextUtilities, TAG_IDLIB_SINGLETON > idTextUtil;

#endif // !__TEXT_UTILITIES_H__
