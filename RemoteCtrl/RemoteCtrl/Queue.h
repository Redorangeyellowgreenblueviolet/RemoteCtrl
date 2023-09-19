#pragma once
#include <list>
template<class T>
class CQueue
{//线程安全的队列, iocp实现
public:
	CQueue();
	~CQueue();
	bool PushBack(const T& data);
	bool PopFront(T& data);
	size_t Size();
	void Clear();
private:
	static void threadEntry(void* arg);
	void threadMain();


private:
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
public:
	enum{	
		QPush,
		QPop,
		QSize,
		QClear
	};
	typedef struct IocpParam {
		int nOperator; //操作
		std::string strData; //数据
		HANDLE hEvent; //pop操作需要
		IocpParam(int op, const char* sData) {
			nOperator = op;
			strData = sData;
		}
		IocpParam() {
			nOperator = -1;
		}
	}POSTPARAM; //post parameter用于投递消息的结构体
};

