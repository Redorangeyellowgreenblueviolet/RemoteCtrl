#pragma once
#include "pch.h"
#include "framework.h"
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
		m_hCompletionPort = INVALID_HANDLE_VALUE;
		if (m_hCompletionPort) {
			m_hThread = (HANDLE)_beginthread(&CQueue<T>::threadEntry, 0, m_hCompletionPort);
		}
	}
	~CQueue() {
		if (m_atom == true) return;
		m_atom = true;
		HANDLE hTemp = m_hCompletionPort;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);
		m_hCompletionPort = NULL;
		CloseHandle(hTemp);
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
	bool PopFront(T& data) {
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
private:
	static void threadEntry(void* arg) {
		CQueue<T>* thiz = (CQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}
	void DealParam(POSTPARAM* pParam) {
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

	void threadMain() {
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
		CloseHandle(m_hCompletionPort);
	}

private:
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_atom;
};

