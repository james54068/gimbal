#include "sd.h"			   
#include "spi.h"

u8  SD_Type=0;//SD卡類型 

//依照SD Card接到的SPI做讀寫動做
//data:要寫入的數據
//return:讀到的數據
u8 SD_SPI_ReadWriteByte(u8 data)
{
	return SPIx_ReadWriteByte(data);
}
//SD初始化的時候需要使用低速設定
void SD_SPI_SpeedLow(void)
{
 	SPIx_SetSpeed(SPI_BaudRatePrescaler_256);//設置到低速模式	
}
//SD正常運作時使用高速
void SD_SPI_SpeedHigh(void)
{
 	SPIx_SetSpeed(SPI_BaudRatePrescaler_8);//設置道高速	
}
//SPI外設初始化
void SD_SPI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(	SD_CS_CLK,ENABLE);	 

    GPIO_InitStructure.GPIO_Pin = SD_CS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;//Push-pull
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SD_CS_PORT, &GPIO_InitStructure);
	Clr_SD_CS;
	SPIx_Init();
	Set_SD_CS;
}
///////////////////////////////////////////////////////////////////////////////////
//取消SD Card選擇
void SD_DisSelect(void)
{
	Set_SD_CS;
	SD_SPI_ReadWriteByte(0xff);//空操作
}
//選擇SD Card，等待卡準備完成進行下一個動作
//Retrun:0,成功;1,失敗;
u8 SD_Select(void)
{
	Clr_SD_CS;
	if(SD_WaitReady()==0)return 0;//等待成功
	//
	SD_DisSelect();
	return 1;//等待失敗
}
//等待SD Card準備好
//return:0,準備好;其他值,失敗
u8 SD_WaitReady(void)
{
	u32 t=0;
	do
	{
		if(SD_SPI_ReadWriteByte(0XFF)==0XFF)return 0;//OK
		t++;		  	
	}while(t<0XFFFFFF);//失敗 
	return 1;
}
//等待SD卡回應
//Response:要得到的回應值
//return:0,成功得到了該回應值
//     其他,得到回應值失敗
u8 SD_GetResponse(u8 Response)
{
	u16 Count=0xFFF;//等待次數	   						  
	while ((SD_SPI_ReadWriteByte(0XFF)!=Response)&&Count)Count--;//等待得到準確的回應  	  
	if (Count==0)return MSD_RESPONSE_FAILURE;//得到回應失敗   
	else return MSD_RESPONSE_NO_ERROR;//正確回應
}
//從SD卡讀取一個數據包的內容��
//buf:數據緩衝區⑹
//len:要讀取的數據長度
//Retrun:0,成功;1,失敗;	
u8 SD_RecvData(u8*buf,u16 len)
{			  	  
	if(SD_GetResponse(0xFE))return 1;
    while(len--)
    {
        *buf=SPIx_ReadWriteByte(0xFF);
        buf++;
    }
 
    SD_SPI_ReadWriteByte(0xFF);
    SD_SPI_ReadWriteByte(0xFF);									  					    
    return 0;
}
//向SD卡謝入一個數據包的內容,512 Bytes
//buf:數據暫存區⑹
//cmd:指令
//Retrun:0,成功;1,失敗;		
u8 SD_SendBlock(u8*buf,u8 cmd)
{	
	u16 t;		  	  
	if(SD_WaitReady())return 1;//脹渾袧掘囮虴
	SD_SPI_ReadWriteByte(cmd);
	if(cmd!=0XFD)//祥岆賦旰硌鍔
	{
		for(t=0;t<512;t++)SPIx_ReadWriteByte(buf[t]);//枑詢厒僅,熬屾滲杅換統奀潔
	    SD_SPI_ReadWriteByte(0xFF);//綺謹crc
	    SD_SPI_ReadWriteByte(0xFF);
		t=SD_SPI_ReadWriteByte(0xFF);//諉彶砒茼
		if((t&0x1F)!=0x05)return 2;//砒茼渣昫									  					    
	}						 									  					    
    return 0;//迡�貐伄�
}

//向SD卡發送一個命令
//輸入��: u8 cmd   命令 
//      u32 arg  命令參數
//      u8 crc   CRC檢查值	   
//返回值:SD卡回傳的值														  
u8 SD_SendCmd(u8 cmd, u32 arg, u8 crc)
{
    u8 r1;	
	u8 Retry=0; 
	SD_DisSelect();//取消片選
	if(SD_Select())return 0XFF;//片選失敗 
	//楷冞
    SD_SPI_ReadWriteByte(cmd | 0x40);//分別寫入命令
    SD_SPI_ReadWriteByte(arg >> 24);
    SD_SPI_ReadWriteByte(arg >> 16);
    SD_SPI_ReadWriteByte(arg >> 8);
    SD_SPI_ReadWriteByte(arg);	  
    SD_SPI_ReadWriteByte(crc); 
	if(cmd==CMD12)SD_SPI_ReadWriteByte(0xff);//Skip a stuff byte when stop reading
    //等待SD卡回應，或是超時退出
	Retry=0X1F;
	do
	{
		r1=SD_SPI_ReadWriteByte(0xFF);
	}while((r1&0X80) && Retry--);	 
	//返回狀態值
    return r1;
}		    																			  
//讀取SD卡的CID信息，包括制造商信息
//輸入: u8 *cid_data(存放CID的內存，至少16Byte）	  
//返回值:0：NO_ERR
//		 1:錯誤														   
u8 SD_GetCID(u8 *cid_data)
{
    u8 r1;	   
    //發送CMD10命令,讀CID
    r1=SD_SendCmd(CMD10,0,0x01);
    if(r1==0x00)
	{
		r1=SD_RecvData(cid_data,16);//接收16個Byte數據	 
    }
	SD_DisSelect();
	if(r1)return 1;
	else return 0;
}																				  
//讀取SD卡的CSD信息，包含容量與速度信息
//輸入: u8 *cid_data(存放CID的內存，至少16Byte）	  
//返回值:0：NO_ERR
//		 1:錯誤														   
u8 SD_GetCSD(u8 *csd_data)
{
    u8 r1;	 
    r1=SD_SendCmd(CMD9,0,0x01);//發送CMD9命令，讀CSD
    if(r1==0)
	{
    	r1=SD_RecvData(csd_data, 16);//接收16個Byte數據
    }
	SD_DisSelect();
	if(r1)return 1;
	else return 0;
}  
//獲取SD卡總扇區數量   
//返回值:0,讀取容量出錯
//       其他,SD卡容量(扇區數/512Byte)
//每個扇區的Byte數必定為512，如果不是512則初始不能通過														  
u32 SD_GetSectorCount(void)
{
    u8 csd[16];
    u32 Capacity;  
    u8 n;
	u16 csize;  					    
	//讀取CSD信息，如果中間出錯則返回0
    if(SD_GetCSD(csd)!=0) return 0;	    
    //如果維SDHC卡，按照下面方式計算
    if((csd[0]&0xC0)==0x40)	 //V2.00的卡
    {	
		csize = csd[9] + ((u16)csd[8] << 8) + 1;
		Capacity = (u32)csize << 10;//扇區數	 		   
    }else//V1.XX的卡
    {	
		n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
		csize = (csd[8] >> 6) + ((u16)csd[7] << 2) + ((u16)(csd[6] & 3) << 10) + 1;
		Capacity= (u32)csize << (n - 9);//得到扇區數   
    }
    return Capacity;
}
u8 SD_Idle_Sta(void)
{
	u16 i;
	u8 retry;	   	  
    for(i=0;i<0xf00;i++);	 
    for(i=0;i<10;i++)SPIx_ReadWriteByte(0xFF); 
    retry = 0;
    do
    {	   
        i = SD_SendCmd(CMD0, 0, 0x95);
        retry++;
    }while((i!=0x01)&&(retry<200));
    if(retry==200)return 1; 
	return 0;				  
}
//初始化SD卡
u8 SD_Initialize(void)
{
    u8 r1;      // 存放SD卡返回值
    u16 retry;  // 用來進行超時計數
    u8 buf[4];  
	u16 i;

	SD_SPI_Init();		//初始化操作IO
 	SD_SPI_SpeedLow();	//設置到低速模式 	  
	for(i=0;i<10;i++)SD_SPI_ReadWriteByte(0XFF);//最少發送74個脈衝
	retry=20;
	do
	{
		r1=SD_SendCmd(CMD0,0,0x95);//進入IDLE狀態
	}while((r1!=0X01) && retry--);
 	SD_Type=0;//預設沒有插入SD卡
	if(r1==0X01)
	{
		if(SD_SendCmd(CMD8,0x1AA,0x87)==1)//SD V2.0
		{
			for(i=0;i<4;i++)buf[i]=SD_SPI_ReadWriteByte(0XFF);	//Get trailing return value of R7 resp
			if(buf[2]==0X01&&buf[3]==0XAA)//SD卡是否支元2.7~3.6V
			{
				retry=0XFFFE;
				do
				{
					SD_SendCmd(CMD55,0,0X01);	//發送CMD55
					r1=SD_SendCmd(CMD41,0x40000000,0X01);//發送CMD41
				}while(r1&&retry--);
				if(retry&&SD_SendCmd(CMD58,0,0X01)==0)//鑑別SD2.0卡
				{
					for(i=0;i<4;i++)buf[i]=SD_SPI_ReadWriteByte(0XFF);//得到的OCR值
					if(buf[0]&0x40)SD_Type=SD_TYPE_V2HC;    //檢茶CCS
					else SD_Type=SD_TYPE_V2;   
				}
			}
		}else//SD V1.x/ MMC	V3
		{
			SD_SendCmd(CMD55,0,0X01);	//發送CMD55
			r1=SD_SendCmd(CMD41,0,0X01);//發送CMD41
			if(r1<=1)
			{		
				SD_Type=SD_TYPE_V1;
				retry=0XFFFE;
				do //脹渾豖堤IDLE耀宒
				{
					SD_SendCmd(CMD55,0,0X01);	//發送CMD55
					r1=SD_SendCmd(CMD41,0,0X01);//發送CMD41
				}while(r1&&retry--);
			}else
			{
				SD_Type=SD_TYPE_MMC;//MMC V3
				retry=0XFFFE;
				do //脹渾豖堤IDLE耀宒
				{											    
					r1=SD_SendCmd(CMD1,0,0X01);//發送CMD1
				}while(r1&&retry--);  
			}
			if(retry==0||SD_SendCmd(CMD16,512,0X01)!=0)SD_Type=SD_TYPE_ERR;//錯誤的卡
		}
	}
	SD_DisSelect();//取消片選
	SD_SPI_SpeedHigh();//高度模式
	if(SD_Type)return 0;
	else if(r1)return r1; 	   
	return 0xaa;//其他模式
}
 
//讀SD卡
//buf:數據暫存區
//sector:扇區⑹
//cnt:扇區數
//return:0,ok;其他,失敗.
u8 SD_ReadDisk(u8 *buf,u32 sector,u8 cnt)
{
	u8 r1;
	if(SD_Type!=SD_TYPE_V2HC)sector <<= 9;
	if(cnt==1)
	{
		r1=SD_SendCmd(CMD17,sector,0X01);
		if(r1==0)
		{
			r1=SD_RecvData(buf,512);  
		}
	}else
	{
		r1=SD_SendCmd(CMD18,sector,0X01);//連續命令
		do
		{
			r1=SD_RecvData(buf,512);//接收512Bytes	 
			buf+=512;  
		}while(--cnt && r1==0); 	
		SD_SendCmd(CMD12,0,0X01);	//發送停止命令
	}   
	SD_DisSelect();//取消片選
	return r1;//
}

//寫SD卡
//buf:數據暫存區⑹
//sector:起始扇區⑹
//cnt:扇區數
//return:0,ok;其他,失敗.

u8 SD_WriteDisk(u8 *buf,u32 sector,u8 cnt)
{
	u8 r1;
	if(SD_Type!=SD_TYPE_V2HC)sector *= 512;
	if(cnt==1)
	{
		r1=SD_SendCmd(CMD24,sector,0X01);
		if(r1==0)
		{	
			r1=SD_SendBlock(buf,0xFE);	   
		}
	}else
	{
		if(SD_Type!=SD_TYPE_MMC)
		{
			SD_SendCmd(CMD55,0,0X01);	
			SD_SendCmd(CMD23,cnt,0X01);	
		}
 		r1=SD_SendCmd(CMD25,sector,0X01);
		if(r1==0)
		{
			do
			{
				r1=SD_SendBlock(buf,0xFC);
				buf+=512;  
			}while(--cnt && r1==0);
			r1=SD_SendBlock(0,0xFD);
		}
	}   
	SD_DisSelect();
	return r1;
}