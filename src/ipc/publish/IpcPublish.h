#ifndef __IPCPUBLISH_H__
#define __IPCPUBLISH_H__
#include <Unknwn.h>


[
	uuid("E6E71F47-3527-4570-9E06-E9C264A7DDB4")
]
interface IPCClient : public IUnknown
{
	virtual int init() = 0;
};


[
	uuid("3A8A5498-A9F5-454B-BB6F-694EEBDF0C6B")
]
interface IPCClient : public IUnknown
{
	virtual int init() = 0;
};


#endif