#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push)
#pragma pack(1)
// 设为 1 字节对齐 去除补全位
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}

	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sSum += strData[j] & 0xFF;
		}
	}

	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) { //Qu
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}

	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				// 包头
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) {// 包数据可能不全
			nSize = 0;
			return;
		}

		nLength = *(DWORD*)(pData + i);
		i += 4;
		if (nLength + i > nSize) { //包未完全收到
			nSize = 0;
			return;
		}

		sCmd = *(WORD*)(pData + i);
		i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}

		sSum = *(WORD*)(pData + i);
		i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {
			nSize = i;
			return;
		}
		nSize = 0;
	}

	int Size() {//包数据大小
		return nLength + 6;
	}

	const char* Data() {//整个包
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		// 赋值
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}


	~CPacket() {}

public:
	WORD sHead; // 固定头部 FEFF
	DWORD nLength; // 包长度 控制命令--校验和
	WORD sCmd; // 控制命令
	std::string strData; //包数据
	WORD sSum; //和校验
	std::string strOut; // 全部
};

#pragma pack(pop)


typedef struct MOUSEEVENT { // 鼠标描述
	MOUSEEVENT() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; // 点击 移动 双击
	WORD nButton; // 左键 右键 中键
	POINT ptXY; // 坐标
}MOUSEEV, * PMOUSEEV;


typedef struct file_info {
	file_info() {
		IsInvalid = 0;
		IsDirectory = 0;
		HasNext = 1;
		memset(szFileName, 0, sizeof(szFileName));
	}
	char szFileName[256]; //文件名
	bool IsInvalid; //是否无效 0有效
	bool IsDirectory; //是否为目录 0不是 1是
	bool HasNext; //是否有后续 0 没有 1有
}FILEINFO, * PFILEINFO;