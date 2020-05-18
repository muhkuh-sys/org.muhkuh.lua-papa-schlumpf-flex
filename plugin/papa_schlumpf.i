%module papa_schlumpf

%include <stdint.i>

#ifdef SWIGLUA
%include "lua_fnptr.i"
#endif


%include muhkuh_typemaps.i


%typemap(in, numinputs=0) PUL_ARGUMENT_OUT
%{
	unsigned long ulArgument_$argnum;
	$1 = &ulArgument_$argnum;
%}
%typemap(argout) PUL_ARGUMENT_OUT
%{
	lua_pushnumber(L, ulArgument_$argnum);
	++SWIG_arg;
%}


%typemap(in, numinputs=0) PPC_ARGUMENT_OUT
%{
	char *pcArgument_$argnum;
	$1 = &pcArgument_$argnum;
%}
%typemap(argout) PPC_ARGUMENT_OUT
%{
	lua_pushstring(L, pcArgument_$argnum);
	free(pcArgument_$argnum);
	++SWIG_arg;
%}


%typemap(out) RESULT_INT_TRUE_OR_NIL_WITH_ERR
%{
	if( $1>=0 )
	{
		lua_pushboolean(L, 1);
		SWIG_arg = 1;
	}
	else
	{
		lua_pushnil(L);
		lua_pushstring(L, arg1->get_error_string($1));
		SWIG_arg = 2;
	}
%}


%typemap(out) RESULT_INT_NOTHING_OR_NIL_WITH_ERR
%{
	if( $1<0 )
	{
		lua_pushnil(L);
		lua_pushstring(L, arg1->get_error_string($1));
		SWIG_arg = 2;
	}
	else
	{
%}
%typemap(ret) RESULT_INT_NOTHING_OR_NIL_WITH_ERR
%{
	}
%}


%typemap(out) RESULT_INT_INT_OR_NIL_WITH_ERR
%{
	if( $1>=0 )
	{
		lua_pushnumber(L, $1);
		SWIG_arg = 1;
	}
	else
	{
		lua_pushnil(L);
		lua_pushstring(L, arg1->get_error_string($1));
		SWIG_arg = 2;
	}
%}


%include "papa_schlumpf.h"

%{
	#include "papa_schlumpf.h"
%}
