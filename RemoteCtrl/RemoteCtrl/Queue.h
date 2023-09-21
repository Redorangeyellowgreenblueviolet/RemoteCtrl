#pragma once
#include "pch.h"
#include "framework.h"
#include "Thread.h"
#include <atomic>
#include <list>
template<class T>
class CQueue
{//线程安全的队列, iocp实现
public:
	enum {
		QNone,
		QPush,
		QPop,
		QSize,
		QClear
	};
	typedef struct IocpParam {
		size_t nOperator; //操作
		T Data; //数据
		HANDLE hEvent; //pop操作需要
		IocpParam(int op, const T& data, HANDLE hEv = NULL) {
			nOperator = op;
			Data = data;
			hEvent = hEv;
		}
		IocpParam() {
			nOperator = QNone;
		}
	}POSTPARAM; //post parameter用于投递消息的结构体
public:
	CQueue() {
		m_atom = false;
		m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompletionPort) {
			m_hThread = (HANDLE)_beginthread(&CQueue<T>::threadEntry, 
				0, this);
			Sleep(1);
		}
	}
	virtual ~CQueue() {
		if (m_atom == true) return;
		m_atom = true;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);
		if (m_hCompletionPort) {
			HANDLE hTemp = m_hCompletionPort;
			m_hCompletionPort = NULL;
			CloseHandle(hTemp);
		}

	}
	bool PushBack(const T& data) {
		IocpParam* pParam = new IocpParam(QPush, data);
		if (m_atom == true) {
			delete pParam;
			return false; 
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, 
			sizeof(POSTPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam;
		return ret;
	}
	virtual bool PopFront(T& data) {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(QPop, data, hEvent);
		if (m_atom == true) {
			if (hEvent) CloseHandle(hEvent);
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort,
			sizeof(POSTPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}
		ret = (WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0);
		if (ret) {
			data = Param.Data;
		}
		return ret;
	}
	size_t Size() {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(QSize, T(), hEvent);
		if (m_atom == true) { 
			if (hEvent) CloseHandle(hEvent);
			return -1; 
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort,
			sizeof(POSTPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return -1;
		}
		ret = (WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0);
		if (ret) {
			return Param.nOperator;
		}
		return -1;
	}
	bool Clear() {
		if (m_atom == true) return false;
		IocpParam* pParam = new IocpParam(QClear, T());
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort,
			sizeof(POSTPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam;
		return true;
	}
protected:
	static void threadEntry(void* arg) {
		CQueue<T>* thiz = (CQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}
	virtual void DealParam(POSTPARAM* pParam) {
		switch (pParam->nOperator)
		{
		case QPush:
			m_lstData.push_back(pParam->Data);
			delete pParam;
			break;
		case QPop:
			if (m_lstData.size() > 0) {
				pParam->Data = m_lstData.front();
				m_lstData.pop_front();
			}
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent);
			}
			break;
		case QSize:
			pParam->nOperator = m_lstData.size();
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent);
			}
			break;
		case QClear:
			m_lstData.clear();
			delete pParam; //释放内存
			break;
		default:
			OutputDebugStringA("unknown operate\r\n");
			break;
		}
	}

	virtual void threadMain() {
		POSTPARAM* pParam = NULL;
		DWORD dwTransferred = 0;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* pOverlapped = NULL;
		while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, 
			&CompletionKey, &pOverlapped, INFINITE))
		{
			if (dwTransferred == 0 || CompletionKey == NULL) {
				printf("thread is prepare to exit\r\n");
				break;
			}
			pParam = (POSTPARAM*)CompletionKey;
			DealParam(pParam);
		}

		while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred,
			&CompletionKey, &pOverlapped, 0))
		{
			if (dwTransferred == 0 || CompletionKey == NULL) {
				printf("thread is prepare to exit\r\n");
				break;
			}
			pParam = (POSTPARAM*)CompletionKey;
			DealParam(pParam);
		}
		HANDLE hTemp = m_hCompletionPort;
		m_hCompletionPort = NULL;
		CloseHandle(hTemp);
	}

protected:
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_atom;
};





template<class T>
class CSendQueue : public CQueue<T>, public ThreadFuncBase
{
public:
	typedef int (ThreadFuncBase::* ECALLBACK)(T &data);
	CSendQueue(ThreadFuncBase* obj, ECALLBACK callback)
		:CQueue<T>(), m_base(obj),m_callback(callback)
	{
		m_thread.Start();
		m_thread.UpdateWorker(::ThreadWorker(this, (FUNCTYPE)&CSendQueue<T>::threadTick));
	}
	virtual ~CSendQueue() {
		m_base = NULL;
		m_callback = NULL;
		m_thread.Stop();
	}
protected:
	virtual bool PopFront(T& data) {
		return false;
	}

	bool PopFront()
	{
		typename CQueue<T>::IocpParam* Param  = 
			new typename CQueue<T>::IocpParam(CQueue<T>::QPop, T());
		if (CQueue<T>::m_atom == true) {
			delete Param;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(CQueue<T>::m_hCompletionPort,
			sizeof(*Param), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			delete Param;
			return false;
		}
		return ret;
	}

	int threadTick() {
		if (WaitForSingleObject(CQueue<T>::m_hThread, 0) != WAIT_TIMEOUT) {
			return 0;
		}
		if (CQueue<T>::m_lstData.size() > 0) {
			PopFront();
		}
		Sleep(1);
		return 0;
	}

	virtual void DealParam(typename CQueue<T>::POSTPARAM* pParam) {
		switch (pParam->nOperator)
		{
		case CQueue<T>::QPush:
			CQueue<T>::m_lstData.push_back(pParam->Data);
			delete pParam;
			break;
		case CQueue<T>::QPop:
			if (CQueue<T>::m_lstData.size() > 0) {
				pParam->Data = CQueue<T>::m_lstData.front();
				if((m_base->*m_callback)(pParam->Data)==0)
					CQueue<T>::m_lstData.pop_front();
			}
			delete pParam;
			break;
		case CQueue<T>::QSize:
			pParam->nOperator = CQueue<T>::m_lstData.size();
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent);
			}
			break;
		case CQueue<T>::QClear:
			CQueue<T>::m_lstData.clear();
			delete pParam; //释放内存
			break;
		default:
			OutputDebugStringA("unknown operate\r\n");
			break;
		}
	}
private:
	ThreadFuncBase* m_base;
	ECALLBACK m_callback;
	CThread m_thread;
};

typedef CSendQueue<std::vector<char>>::ECALLBACK SENDCALLBACK;