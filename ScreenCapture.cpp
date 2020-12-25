/**
 * 对某个区域进行截屏,返回位图句柄
 * 参考链接: https://blog.csdn.net/u013105439/article/details/53270737
 * 其他链接后续可阅读: https://docs.microsoft.com/en-us/windows/win32/gdi/storing-an-image
 */
HBITMAP CaptureScreen( LPRECT lpRect )
{
	HDC hdc = NULL;
	HDC hdcMem = NULL;
	HBITMAP hemfCopy = NULL;
	HWND hwndScr = NULL;
	int dstcx = 0;
	int	dstcy = 0;
	if(lpRect)
	{
		dstcx = lpRect->right - lpRect->left ;
		dstcy = lpRect->bottom - lpRect->top ;
	}
	else
	{
		dstcx = GetSystemMetrics(SM_CXSCREEN) ;
		dstcy = GetSystemMetrics(SM_CYSCREEN) ;
	}
	if (LockWindowUpdate(hwndScr = GetDesktopWindow ()))
	{
		hdc  = GetDCEx (hwndScr, NULL, DCX_CACHE | DCX_LOCKWINDOWUPDATE) ;
		hdcMem = CreateCompatibleDC (hdc) ;
		if(NULL==hdcMem)
		{
			ReleaseDC (hwndScr, hdc) ;
			LockWindowUpdate (NULL) ;
			return NULL;
		}
		hemfCopy = CreateCompatibleBitmap (hdc, abs(dstcx), abs(dstcy)) ;
		if(NULL==hemfCopy)
		{
			DeleteDC (hdcMem) ;
			ReleaseDC (hwndScr, hdc) ;
			LockWindowUpdate (NULL) ;
			return NULL;
		}
		SelectObject (hdcMem, hemfCopy) ;
		if(lpRect)
		{
			StretchBlt (hdcMem, 0, 0, abs(dstcx), abs(dstcy), 
				hdc, lpRect->left, lpRect->top, dstcx, dstcy, SRCCOPY|CAPTUREBLT) ;
		}
		else
		{
			BitBlt (hdcMem, 0, 0, dstcx, dstcy, 
				hdc, 0, 0, SRCCOPY|CAPTUREBLT) ;
		}
		DeleteDC (hdcMem) ;
		ReleaseDC (hwndScr, hdc) ;
		LockWindowUpdate (NULL) ;

		return hemfCopy;
	}
	return NULL;
}


/**
 * 将位图文件保存到本地目录
 * 位图信息介绍: https://zh.wikipedia.org/wiki/BMP
 */
BOOL CSaveBmp( HBITMAP hBitmap,CString FileName )
{
	HDC hDC = NULL;         
	//当前分辨率下每象素所占字节数         
	int iBits = 0;         
	//位图中每象素所占字节数         
	WORD wBitCount=0;         
	//定义调色板大小,位图中像素字节大小,位图文件大小,写入文件字节数             
	DWORD dwPaletteSize=0,dwBmBitsSize=0,dwDIBSize=0,dwWritten=0;             
	//位图属性结构             
	BITMAP Bitmap={0};                 
	//位图文件头结构         
	BITMAPFILEHEADER bmfHdr={0};                 
	//位图信息头结构             
	BITMAPINFOHEADER bi={0};                 
	//指向位图信息头结构                 
	LPBITMAPINFOHEADER lpbi={0};                 
	//定义文件，分配内存句柄，调色板句柄             
	HANDLE fh=NULL,hDib=NULL,hPal=NULL,hOldPal=NULL;             

	//计算位图文件每个像素所占字节数             
	hDC  = CreateDC(TEXT("DISPLAY"),NULL,   NULL,   NULL);         
	iBits  = GetDeviceCaps(hDC,BITSPIXEL)*GetDeviceCaps(hDC,PLANES);             
	DeleteDC(hDC);             
	if(iBits <=  1)                                                   
		wBitCount = 1;             
	else  if(iBits <=  4)                               
		wBitCount  = 4;             
	else if(iBits <=  8)                               
		wBitCount  = 8;             
	else                                                                                                                               
		wBitCount  = 24;             

	GetObject(hBitmap,   sizeof(Bitmap),   (LPSTR)&Bitmap);         
	bi.biSize= sizeof(BITMAPINFOHEADER);	// 头结构的大小      
	bi.biWidth = Bitmap.bmWidth;			// 位图宽度(单位:像素)
	bi.biHeight =  Bitmap.bmHeight;         // 位图高度
	bi.biPlanes =  1;						// 色彩平面数;只有1为有效值
	bi.biBitCount = wBitCount;				// 每个像素所占位数(也叫色深),通常取值1,4,8,16,24,32
	bi.biCompression= BI_RGB;				// 使用的压缩方法    
	bi.biSizeImage=0;						// 图像大小(原始位图数据大小)
	bi.biXPelsPerMeter = 0;					// 图像的横向分辨率(单位:像素每米)
	bi.biYPelsPerMeter = 0;					// 图像的纵向分辨率
	bi.biClrImportant = 0;					// 调色板的颜色数,为0表示颜色数=2^{色深} 个
	bi.biClrUsed =  0;						// 重要颜色数,为0表示所有颜色都是重要的,通常不使用   

	// 计算存储整张位图所需要的字节数
	dwBmBitsSize  = ((Bitmap.bmWidth *wBitCount+31) / 32)*4* Bitmap.bmHeight;         

	//为位图内容分配内存(字节数+调色板大小+位图信息头大小)             
	hDib  = GlobalAlloc(GHND,dwBmBitsSize+dwPaletteSize+sizeof(BITMAPINFOHEADER));             
	lpbi  = (LPBITMAPINFOHEADER)GlobalLock(hDib);             
	*lpbi  = bi;             

	//     处理调色板                 
	hPal  = GetStockObject(DEFAULT_PALETTE);             
	if (hPal)             
	{             
		hDC  = ::GetDC(NULL);             
		hOldPal = ::SelectPalette(hDC,(HPALETTE)hPal, FALSE);             
		RealizePalette(hDC);             
	}         

	// 获取该调色板下新的像素值  
	// hDC: a handle to the device context
	// hbm: a handle to the bitmap
	// start: the first scan line to retrieve
	// cLines: the number of scan lines to retrieve
	// lpvVits: a pointer to a buffer to receive the bitmap data
	// lpbmi: a pointer to a BITMAPINFO structure that specifies the desired 
	// format for the DIB data
	// usage: the format of the bmiColors member of the BITMAPINFO structure
	// return: if lpvBits is non-NULL and the function succeeds,return the number
	// of scan line copied from the bitmap;
	// if lpvBits is non-NULL and GetDIB successfully fills the BITMAPINFO structure
	// structure,the return value is nonzero
	// fails,return 0
	GetDIBits(hDC,hBitmap, 0,(UINT)Bitmap.bmHeight,  
		(LPSTR)lpbi+ sizeof(BITMAPINFOHEADER)+dwPaletteSize,   
		(BITMAPINFO *)lpbi, DIB_RGB_COLORS);             

	//恢复调色板                 
	if (hOldPal)             
	{             
		::SelectPalette(hDC,   (HPALETTE)hOldPal,   TRUE);             
		RealizePalette(hDC);             
		::ReleaseDC(NULL,   hDC);             
	}             

	//创建位图文件                 
	fh  = CreateFile(FileName,   GENERIC_WRITE,0,   NULL,   CREATE_ALWAYS,           
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,   NULL);             

	if (fh     ==  INVALID_HANDLE_VALUE)         return     FALSE;             

	//     设置位图文件头             
	bmfHdr.bfType  = 0x4D42;     //     "BM"             
	dwDIBSize  = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+dwPaletteSize+dwBmBitsSize;                 
	bmfHdr.bfSize  = dwDIBSize;             
	bmfHdr.bfReserved1  = 0;             
	bmfHdr.bfReserved2  = 0;             
	bmfHdr.bfOffBits  = (DWORD)sizeof(BITMAPFILEHEADER)+(DWORD)sizeof(BITMAPINFOHEADER)+dwPaletteSize;             
	//     写入位图文件头             
	WriteFile(fh,   (LPSTR)&bmfHdr,   sizeof(BITMAPFILEHEADER),   &dwWritten,   NULL);             
	//     写入位图文件其余内容             
	WriteFile(fh,   (LPSTR)lpbi,   dwDIBSize,   &dwWritten,   NULL);             
	//清除                 
	GlobalUnlock(hDib);             
	GlobalFree(hDib);             
	CloseHandle(fh);             

	return     TRUE;
}


bool CScreenCaptureMgr::CaptureScreenPNG(CRect srcRect,CString pngFileName)
{
	HDC hScrDC=CreateDC("DISPLAY",NULL,NULL,NULL);
	HDC hMemDC=CreateCompatibleDC(hScrDC);
	HBITMAP hBitmap=CreateCompatibleBitmap(hScrDC,srcRect.Width(),srcRect.Height());
	HBITMAP hOldBitmap=(HBITMAP)SelectObject(hMemDC,hBitmap);

	BitBlt(hMemDC,0,0,srcRect.Width(),srcRect.Height(),hScrDC,srcRect.left,srcRect.top,SRCCOPY);

	hBitmap=(HBITMAP)SelectObject(hMemDC,hOldBitmap);

	CxImage SnapShootImage;
	SnapShootImage.CreateFromHBITMAP(hBitmap);
	bool bSaveOK = false;

    CString csSavePath = "";
	{
        // TODO 确保保存图片的路径存在，即根据pngFileName来生成保存本地文件的路径，如果目录不存在，则创建它
    }

	

	if (SnapShootImage.IsValid())
	{
		SnapShootImage.SetJpegQuality(80);
		bSaveOK = SnapShootImage.Save(csSavePath.GetBuffer(),CXIMAGE_FORMAT_PNG);// CXIMAGE_FORMAT_JPG);
	}
	DeleteObject((HGDIOBJ)(HBITMAP)(hBitmap));
	DeleteDC(hMemDC);
	DeleteDC(hScrDC);
	return bSaveOK;
}