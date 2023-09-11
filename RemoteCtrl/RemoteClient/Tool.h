#pragma once
#include <Windows.h>
#include <atlimage.h>
#include <string>

class CTool
{
public:
    static void Dump(BYTE* pData, size_t nSize) {

        std::string strOut;
        for (size_t i = 0; i < nSize; i++) {
            char buf[8] = "";
            if (i > 0 && (i % 16 == 0))
                strOut += "\n";
            snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
            strOut += buf;
        }
        strOut += "\n";
        OutputDebugStringA(strOut.c_str());
    }
    static int Bytes2Image(CImage& image, std::string& strBuf) {
		// ´æÈëCImage
		BYTE* pData = (BYTE*)strBuf.c_str();
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
		if (hMem == NULL) {
			TRACE(_T("ÄÚ´æ²»×ã\r\n"));
			Sleep(1);
			return -1;
		}
		IStream* pStream = NULL;
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
		if (hRet == S_OK) {
			ULONG ulength = 0;
			pStream->Write(pData, strBuf.size(), &ulength);
			LARGE_INTEGER bg{ 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);

			if ((HBITMAP)image != NULL)
				image.Destroy();

			image.Load(pStream);
		}
		return hRet;
    }
};

