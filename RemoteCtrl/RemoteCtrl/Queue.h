#pragma once
#include <list>
template<class T>
class CQueue
{//�̰߳�ȫ�Ķ���, iocpʵ��
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
		int nOperator; //����
		std::string strData; //����
		HANDLE hEvent; //pop������Ҫ
		IocpParam(int op, const char* sData) {
			nOperator = op;
			strData = sData;
		}
		IocpParam() {
			nOperator = -1;
		}
	}POSTPARAM; //post parameter����Ͷ����Ϣ�Ľṹ��
};

