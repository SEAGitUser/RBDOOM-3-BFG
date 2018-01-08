// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SINGLETON_H__
#define __SINGLETON_H__

//static idLock singletonLock;
static idSysMutex singletonLock;

template < typename T, memTag_t Tag > class idSingleton {
public:
	idSingleton() { ; }

private:
	~idSingleton() {
		DestroyInstance();
	}

public:
	static T& GetInstance() {
		if ( !instance ) {

			//singletonLock.Acquire();
			singletonLock.Lock();

			if ( !instance ) {
				instance = new( Tag ) T;
			}

			//singletonLock.Release();
			singletonLock.Unlock();
		}
		return *instance;
	}

	static void DestroyInstance() {
		if ( instance ) {
			delete instance;
			instance = nullptr;
		}
	}

private:
	static T* instance;
};

template < typename T, memTag_t Tag > 
T* idSingleton< T, Tag >::instance = nullptr;

#endif /* !__SINGLETON_H__ */
