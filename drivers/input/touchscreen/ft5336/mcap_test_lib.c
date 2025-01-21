#include <linux/kernel.h>
#include <linux/string.h>

#include "mcap_test_lib.h"
#include "ini.h"

#include <linux/delay.h>
#include <linux/slab.h>

struct Test_ConfigParam_MCap{
	 short rawdata_max;
	 short rawdata_min;
	 short diffdata_max;
	 short diffdata_min;

	 char changevol_level;
	 char changeoffset_level;
	 char Set_Vol;
	 char start_vol;
	 char end_vol;

     	 short valid_node[TX_NUM_MAX][RX_NUM_MAX];
	 short rawdata_max_node[TX_NUM_MAX][RX_NUM_MAX];
	 short rawdata_min_node[TX_NUM_MAX][RX_NUM_MAX];
	 short diffdata_max_node[TX_NUM_MAX][RX_NUM_MAX];
	 short diffdata_min_node[TX_NUM_MAX][RX_NUM_MAX];
};

/*test section*/
#define Section_TestItem 	"TestItem"
#define Section_BaseSet "BaseSet"
#define Section_SpecialSet "SpecialSet"
#define Section_INVALID_NODE "INVALID_NODE"

#define Item_Chip_Ver "Chip_Ver"
#define Item_RawData_Min "RawData_Min"
#define Item_RawData_Max "RawData_Max"
#define Item_VolDifferData_Min "VolDifferData_Min"
#define Item_VolDifferData_Max "VolDifferData_Max"

#define Check_RAWDATA_MODIFY "RAWDATA_MODIFY"
#define Item_Deviation_Value "Deviation_Value"
#define Check_Set_Vol "Set_Vol"
#define Item_Start_Vol "Start_Vol"
#define Item_End_Vol "End_Vol"
#define Item_Vol_Level "Vol_Level"
#define Special_RawData_Max_Tx "RawData_Max_Tx"
#define Special_RawData_Min_Tx "RawData_Min_Tx"
#define Special_Panel_Differ_Max_Tx "Panel_Differ_Max_Tx"
#define Special_Panel_Differ_Min_Tx "Panel_Differ_Min_Tx"
#define InvalidNode "InvalidNode"
#define Item_SCAP_RAWDATA_TEST "SCAP_RAWDATA_TEST"

FTS_I2c_Read_Function focal_I2C_Read;
FTS_I2c_Write_Function focal_I2C_write;

boolean bRawdataTest = true;

short iTxNum = 0;
short iRxNum = 0;

unsigned char Reg_VolAddr = 0x05;

struct Test_ConfigParam_MCap g_testparam;
char *g_testparamstring = NULL;

static short rawdata[TX_NUM_MAX][RX_NUM_MAX];
static short diffdata[TX_NUM_MAX][RX_NUM_MAX];
static short tmprawdata[TX_NUM_MAX][RX_NUM_MAX];
static short bs_diffdata[TX_NUM_MAX][RX_NUM_MAX];
static char change_rxoffset[RX_NUM_MAX];
static char m_RxOffset[RX_NUM_MAX];
static char m_TxOffset[TX_NUM_MAX];

void TestTp(void);
void Ft5336_TestRawDataAndDiff(void);
static int StartScan(void);
int SetDriverVol(unsigned char vol);
char GetDriverVol(void);
void SetTxRxNum(short txnum,short rxnum);
short GetTxNum(void);
short GetRxNum(void);
char GetOffsetTx(unsigned char txindex);
char GetOffsetRx(unsigned char rxindex);
void SetOffsetTx(unsigned char txindex,unsigned char offset);
void SetOffsetRx(unsigned char rxindex,unsigned char offset);
void GetRawData(short RawData[TX_NUM_MAX][RX_NUM_MAX]);
boolean StartTestTP(void);

static void focal_msleep(int ms)
{
	msleep(ms);
}
int SetParamData(char * TestParamData)
{
	g_testparamstring = TestParamData;
	GetTestParam();
	return 0;
}
void FreeTestParamData(void)
{
	if(g_testparamstring)
		kfree(g_testparamstring);

	g_testparamstring = NULL;
}

static short focal_abs(short value)
{
	short absvalue = 0;
	if(value > 0)
		absvalue = value;
	else
		absvalue = 0 - value;

	return absvalue;
}
int GetParamValue(char *section, char *ItemName, int defaultvalue) 
{
	int paramvalue = defaultvalue;
	char value[512];
	memset(value , 0, sizeof(value));
	if(ini_get_key(g_testparamstring, section, ItemName, value) < 0) {
		return paramvalue;
	} else {
		paramvalue = atoi(value);
	}
	
	return paramvalue;
}

int GetParamString(char *section, char *ItemName, char *defaultvalue) {
	char value[512];
	int len = 0;
	memset(value , 0x00, sizeof(value));
	if(ini_get_key(g_testparamstring, section, ItemName, value) < 0) {
		return 0;
	} else {
		len = sprintf(defaultvalue, "%s", value);
	}

	return len;
}

void GetTestParam(void)
{
	char str_tmp[128], str_node[64], str_value[512];
	int j, index, valuelen = 0, i = 0, k = 0;

	memset(str_tmp, 0, sizeof(str_tmp));
	memset(str_node, 0, sizeof(str_node));
	memset(str_value, 0, sizeof(str_value));
#if 1
	g_testparam.rawdata_min = GetParamValue(Section_BaseSet, Item_RawData_Min, 5500);
	g_testparam.rawdata_max = GetParamValue(Section_BaseSet, Item_RawData_Max, 9500);
	g_testparam.diffdata_min = GetParamValue(Section_BaseSet, Item_VolDifferData_Min, 10);
	g_testparam.diffdata_max = GetParamValue(Section_BaseSet, Item_VolDifferData_Max, 500);

	for(i=0; i<TX_NUM_MAX; i++) 
	{
		for(j=0; j<RX_NUM_MAX; j++) 
		{
			g_testparam.rawdata_min_node[i][j] = g_testparam.rawdata_min;
			g_testparam.rawdata_max_node[i][j] = g_testparam.rawdata_max;
			g_testparam.diffdata_min_node[i][j] = g_testparam.diffdata_min;
			g_testparam.diffdata_max_node[i][j] = g_testparam.diffdata_max;
				
			g_testparam.valid_node[i][j] = 1;
		}
	}

	g_testparam.changevol_level = 2;
	g_testparam.changeoffset_level = 4;
	g_testparam.start_vol = 0;
	g_testparam.end_vol = 2;
	
	g_testparam.Set_Vol = GetParamValue(Section_BaseSet, Check_Set_Vol, 0);
	
	if(g_testparam.Set_Vol > 0)
	{
 	   	g_testparam.start_vol = GetParamValue(Section_BaseSet, Item_Start_Vol, 0);
 	    	g_testparam.end_vol = GetParamValue(Section_BaseSet, Item_End_Vol, 2);

 	}
	else
	{
	    	g_testparam.changevol_level = GetParamValue(Section_BaseSet, Item_Vol_Level, 2);
	}

	for(i=0; i<TX_NUM_MAX; i++) 
	{
		for(j=0; j<RX_NUM_MAX; j++) 
		{
			memset(str_tmp, 0x00, sizeof(str_tmp));
    			sprintf(str_tmp, "%s[%d][%d]",InvalidNode,(i+1),(j+1));
    			g_testparam.valid_node[i][j] = GetParamValue(Section_INVALID_NODE, str_tmp, 1);
		}
	}

	
	
	for(i=0; i<TX_NUM_MAX; i++) 
	{		
        	memset(str_value, 0x00, sizeof(str_value));
       	memset(str_tmp, 0x00, sizeof(str_tmp));
    		sprintf(str_tmp, "%s%d",Special_RawData_Max_Tx,(i+1));

	    	valuelen = GetParamString(Section_SpecialSet, str_tmp, str_value);
	    	if (valuelen > 0) 
	    	{
	    		index = 0;
			k = 0;
	    		memset(str_tmp, 0x00, sizeof(str_tmp));
	    		for(j=0; j<valuelen; j++) 
	    		{
	    			if(',' == str_value[j]) 
	    			{
	    				g_testparam.rawdata_max_node[i][k] = (short)(atoi(str_tmp));
	    				index = 0;
	    				memset(str_tmp, 0x00, sizeof(str_tmp));
	    				k++;
	    			} 
	    			else 
	    			{
	    				if(' ' == str_value[j])
	    					continue;
	    				str_tmp[index] = str_value[j];
	    				index++;
	    			}
	    		}
	    	} 
	    	else 
	    	{
	    		for(j = 0; j < RX_NUM_MAX; j++) 
	    		{
	    			g_testparam.rawdata_max_node[i][j] = g_testparam.rawdata_max;
	    			#ifdef FOCAL_DBG
	    			#endif
	    		}
	    	}

		
	    	memset(str_value, 0x00, sizeof(str_value));
	       memset(str_tmp, 0x00, sizeof(str_tmp));
	    	sprintf(str_tmp, "%s%d",Special_RawData_Min_Tx,(i+1));

	    	valuelen = GetParamString(Section_SpecialSet, str_tmp, str_value);
		
	    	if (valuelen > 0) 
	    	{
	    		index = 0;
			k = 0;
	    		memset(str_tmp, 0x00, sizeof(str_tmp));
	    		for(j=0; j<valuelen; j++) 
	    		{
	    			if(',' == str_value[j]) 
	    			{
	    				g_testparam.rawdata_min_node[i][k] = (short)(atoi(str_tmp));
	    				index = 0;
	    				memset(str_tmp, 0x00, sizeof(str_tmp));
	    				k++;
	    			} 
	    			else 
	    			{
	    				if(' ' == str_value[j])
	    					continue;
	    				str_tmp[index] = str_value[j];
	    				index++;
	    			}
	    		}
	    	} 
	    	else 
	    	{
	    		for(j = 0; j < RX_NUM_MAX; j++) 
	    		{
	    			g_testparam.rawdata_min_node[i][j] = g_testparam.rawdata_min;
	    		}
	    	}

	    	memset(str_value, 0x00, sizeof(str_value));
	       memset(str_tmp, 0x00, sizeof(str_tmp));
	    	sprintf(str_tmp, "%s%d",Special_Panel_Differ_Max_Tx,(i+1));

	    	valuelen = GetParamString(Section_SpecialSet, str_tmp, str_value);
		
	    	if (valuelen > 0) 
	    	{
	    		index = 0;
			k = 0;
	    		memset(str_tmp, 0x00, sizeof(str_tmp));
	    		for(j=0; j<valuelen; j++) 
	    		{
	    			if(',' == str_value[j]) 
	    			{
	    				g_testparam.diffdata_max_node[i][k] = (short)(atoi(str_tmp));
	    				index = 0;
	    				memset(str_tmp, 0x00, sizeof(str_tmp));
	    				k++;
	    			} 
	    			else 
	    			{
	    				if(' ' == str_value[j])
	    					continue;
	    				str_tmp[index] = str_value[j];
	    				index++;
	    			}
	    		}
	    	} 
	    	else 
	    	{
	    		for(j = 0; j < RX_NUM_MAX; j++) 
	    		{
	    			g_testparam.diffdata_max_node[i][j] = g_testparam.diffdata_max;
	    		}
	    	}

	    	memset(str_value, 0x00, sizeof(str_value));
	       memset(str_tmp, 0x00, sizeof(str_tmp));
	    	sprintf(str_tmp, "%s%d",Special_Panel_Differ_Min_Tx,(i+1));

	    	valuelen = GetParamString(Section_SpecialSet, str_tmp, str_value);
		
	    	if (valuelen > 0) 
	    	{
	    		index = 0;
			k = 0;
	    		memset(str_tmp, 0x00, sizeof(str_tmp));
	    		for(j=0; j<valuelen; j++) 
	    		{
	    			if(',' == str_value[j]) 
	    			{
	    				g_testparam.diffdata_min_node[i][k] = (short)(atoi(str_tmp));
	    				index = 0;
	    				memset(str_tmp, 0x00, sizeof(str_tmp));
	    				k++;
	    			} 
	    			else 
	    			{
	    				if(' ' == str_value[j])
	    					continue;
	    				str_tmp[index] = str_value[j];
	    				index++;
	    			}
	    		}
	    	} 
	    	else 
	    	{
	    		for(j = 0; j < RX_NUM_MAX; j++) 
	    		{
	    			g_testparam.diffdata_min_node[i][j] = g_testparam.diffdata_min;
	    		}
	    	}

	    	
	}
	
#endif
}

int Init_I2C_Read_Func(FTS_I2c_Read_Function fpI2C_Read)
{
	focal_I2C_Read = fpI2C_Read;
	return 0;
}

int Init_I2C_Write_Func(FTS_I2c_Write_Function fpI2C_Write)
{
	focal_I2C_write = fpI2C_Write;
	return 0;
}

int ReadReg(unsigned char RegAddr, unsigned char *RegData)
{
	return focal_I2C_Read(&RegAddr, 1, RegData, 1);
}

int WriteReg(unsigned char RegAddr, unsigned char RegData)
{
	unsigned char cmd[2] = {0};
	cmd[0] = RegAddr;
	cmd[1] = RegData;
	return focal_I2C_write(cmd, 2);
}

static int StartScan(void)
{
	int err = 0, i = 0;
	unsigned char regvalue = 0x00;
	
	/*scan*/
	if(WriteReg(0x00,0x40) < 0) {
		FTS_DBG("Enter factory failure\n");
	}
	focal_msleep(100);
	
	err = ReadReg(0x00,&regvalue);
	if (err < 0) 
	{
		FTS_DBG("Enter StartScan %d \n", regvalue);
		return err;
	}
	else 
	{
		regvalue |= 0x80;
		err = WriteReg(0x00,regvalue);
		if (err < 0) 
		{
			return err;
		}
		else 
		{
			for(i=0; i<20; i++) {
				focal_msleep(8);
				err = ReadReg(0x00,&regvalue);
				if (err < 0) 
				{
					return err;
				} 
				else 
				{
					if (0 == (regvalue >> 7)) {
						break;
					}
				}
			}
			if (i >= 20) 
			{
				return -5;
			}
		}
	}
	
	return 0;
}	

int SetDriverVol(unsigned char vol)
{
    return WriteReg(Reg_VolAddr,vol);
}

char GetDriverVol(void)
{
   char vol = 0;
   unsigned char regvalue = 0x00;

   ReadReg(Reg_VolAddr,&regvalue);
   vol = (char)regvalue;

   return vol;
}

void SetTxRxNum(short txnum,short rxnum)
{
    iTxNum = txnum;
    iRxNum = rxnum;
}

short GetTxNum(void)
{
   short txnum = 0;
   unsigned char regvalue = 0x00;

   if(WriteReg(0x00, 0x40) >= 0)
   {
        ReadReg(0x03,&regvalue);
        txnum = (short)regvalue;
   }
   else
   {
        return TX_NUM_MAX;
   }

   return txnum;
}

short GetRxNum(void)
{
   short rxnum = 0;
   unsigned char regvalue = 0x00;

   if(WriteReg(0x00, 0x40) >= 0)
   {
        ReadReg(0x04,&regvalue);
        rxnum = (short)regvalue;
   }
   else
   {
        return RX_NUM_MAX;
   }

   return rxnum;
}

char GetOffsetTx(unsigned char txindex)
{
   char txoffset = 0;
   char regvalue = 0x00;
   
   ReadReg((0xad + txindex),&regvalue);
   txoffset = regvalue;

   return txoffset;
}

char GetOffsetRx(unsigned char rxindex)
{
   char rxoffset = 0;
   char regvalue = 0x00;
   
   ReadReg((0xd6 + rxindex),&regvalue);
   rxoffset = regvalue;

   return rxoffset;
}

void SetOffsetTx(unsigned char txindex,unsigned char offset)
{
   WriteReg((0xad + txindex),offset);
}

void SetOffsetRx(unsigned char rxindex,unsigned char offset)
{  
   WriteReg((0xd6 + rxindex),offset);
}

void GetRawData(short RawData[TX_NUM_MAX][RX_NUM_MAX])
{
	unsigned char LineNum = 0;
	unsigned char I2C_wBuffer[3];
	unsigned char rrawdata[RX_NUM_MAX*2];
	unsigned char j = 0, loop = 0, len = 0, i = 0;
	short mtk_readlen = 8;
	short ByteNum = 0;
	int ReCode = 0;
		
	if(WriteReg(0x00,0x40) >= 0)
	{
		if(StartScan() >= 0)
	        {			
			for(LineNum = 0;LineNum < iTxNum;LineNum++)
	           	 {
	                	I2C_wBuffer[0] = 0x01;
				I2C_wBuffer[1] = LineNum;
				ReCode = focal_I2C_write(I2C_wBuffer, 2);
				ByteNum = iRxNum * 2;

				if (ReCode >= 0) {
					if(ByteNum % mtk_readlen == 0)
						loop = ByteNum / mtk_readlen;
					else
						loop = ByteNum / mtk_readlen + 1;
					for (j = 0; j < loop; j++) 
					{
						len = ByteNum - j * mtk_readlen;
						if (len > mtk_readlen)
							len = mtk_readlen;
						
						I2C_wBuffer[0] = (unsigned char)(0x10 + j * mtk_readlen);
						I2C_wBuffer[1] = len;

						memset(rrawdata, 0x00, sizeof(rrawdata));

						ReCode = focal_I2C_Read(I2C_wBuffer, 2, rrawdata, len);
						
						if (ReCode >= 0) 
						{
							for (i = 0; i < (len >> 1); i++) 
							{
								RawData[LineNum][i+j*mtk_readlen/2] = (short)((unsigned short)(rrawdata[i << 1]) << 8) \
									+ (unsigned short)rrawdata[(i << 1) + 1];
							}
						}
						else 
						{
							FTS_DBG("Get Rawdata failure\n");
							break;
						}
					}
				}

	           	}
	        }
	}
        
}

boolean StartTestTP(void) 
{
	bRawdataTest = true;
	
	TestTp();
	
	return bRawdataTest;
}

void TestTp(void) {
	int i = 0;//, min = 0, max = 0;
	//unsigned char regvalue = 0x00;
	
	bRawdataTest = true;

	if(WriteReg(0x00, 0x40) < 0) {
		FTS_DBG("Enter factory failure\n");
		bRawdataTest = false;
		goto Enter_WorkMode;
	}
	focal_msleep(200);

   	iTxNum = GetTxNum();
    	iRxNum = GetRxNum();

       Ft5336_TestRawDataAndDiff();
	
Enter_WorkMode:	
	//the end, return work mode
	for (i = 0; i < 3; i++) {
		if (WriteReg(0x00, 0x00) >=0)
			break;
		else {
			focal_msleep(200);
		}
	}
}

void Ft5336_TestRawDataAndDiff()
{
    char m_txoff = 0;
    char m_rxoff = 0;
    int i = 0;
    int j = 0;
    char org_vol = 0;
    char end_vol = 0;
    char start_vol = 0;
    char change_vol = 0;
    char Min_Offset = 0;
    char Max_Offset = 0;
    char maxTxOffset = 0;
    char ucChangeCB = g_testparam.changeoffset_level;
    short min_value = 0;
    short max_value = 0;
    
    for(i = 0;i < 3;i++)
    {
        GetRawData(rawdata);
    }

    for(i = 0;i < iTxNum;i++)
    {
        for(j = 0;j < iRxNum;j++)
        {
            if((short)0 == g_testparam.valid_node[i][j])
            {
                continue;
            }

            min_value = g_testparam.rawdata_min_node[i][j];
            max_value = g_testparam.rawdata_max_node[i][j];

            if(rawdata[i][j] < min_value || rawdata[i][j] > max_value)
            {
                bRawdataTest = false;
		  FTS_DBG("rawdata test failure. min_value=%d max_value=%d rawdata[%d][%d]=%d \n", \
				min_value, max_value, i+1, j+1, rawdata[i][j]);
            }
        }
    }

    org_vol = GetDriverVol();
    start_vol = org_vol;
    end_vol = org_vol;

    if(1 == g_testparam.Set_Vol)
    {
        SetDriverVol(g_testparam.start_vol);
        start_vol = g_testparam.start_vol;

        for(i = 0;i < 4;i++)
        {
            GetRawData(rawdata);
        }
    }
    else
    {
        for(i = 0;i < 4;i++)
        {
            GetRawData(rawdata);
        }
    }

    for(i = 0;i < iTxNum;i++)
    {
        m_txoff = GetOffsetTx(i);
        m_TxOffset[i] = m_txoff;

        if(maxTxOffset < m_txoff)
            maxTxOffset = m_txoff;
    }

    for(i = 0;i < iRxNum;i++)
    {
        m_rxoff = GetOffsetRx(i);
        m_RxOffset[i] = m_rxoff;

        if(Min_Offset > m_rxoff)
            Min_Offset = m_rxoff;

        if(Max_Offset < m_rxoff)
            Max_Offset = m_rxoff;
    }

    for(i = 0; i < iRxNum; i++)
    {
    	if(Min_Offset > ucChangeCB)
		{
    		change_rxoffset[i] = 0 - ucChangeCB;
		}
		else if(Min_Offset < ucChangeCB && Max_Offset < 31 - maxTxOffset - ucChangeCB)
		{
			change_rxoffset[i] = ucChangeCB;
		}
		else
		{
			if(m_RxOffset[i] - ucChangeCB > 0)
				change_rxoffset[i] = 0 - ucChangeCB;
			else
				change_rxoffset[i] = ucChangeCB;
		}
    	SetOffsetRx(i, (unsigned char)(m_RxOffset[i] + change_rxoffset[i]));
    }

    focal_msleep(30);

    for(i = 0;i < 4;i++)
    {
        GetRawData(tmprawdata);
    }

    for(i=0; i<iTxNum; i++)
    {
    	for(j=0; j<iRxNum; j++)
    	{
    		bs_diffdata[i][j] = tmprawdata[i][j] - rawdata[i][j];	
    	}
    }
    
    for(i = 0; i<iRxNum; i++)
    {
    	SetOffsetRx(i, (unsigned char)(m_RxOffset[i]));
    }
    focal_msleep(30);

    if(1 == g_testparam.Set_Vol)
    {
        SetDriverVol(g_testparam.end_vol);
        end_vol = g_testparam.end_vol;

        for(i = 0;i < 4;i++)
        {
            GetRawData(diffdata);
        }
    }
    else
    {
       start_vol = org_vol;
			
		if (g_testparam.changevol_level <= 2) 
		{
			if (org_vol <= 1)// ½µÑ¹
			{
				change_vol = org_vol + g_testparam.changevol_level;
			} 
			else 
			{
				change_vol = org_vol - g_testparam.changevol_level;
			}
		}
		else if (3 == g_testparam.changevol_level) 
		{
			if (org_vol <= 2)// ½µÑ¹
				change_vol = org_vol + g_testparam.changevol_level;
			else
				change_vol = org_vol - g_testparam.changevol_level;
		} 
		else 
		{
			if (org_vol <= 3)// ½µÑ¹
				change_vol = org_vol + g_testparam.changevol_level;
			else
				change_vol = org_vol - g_testparam.changevol_level;
		}
		if(change_vol > 7)
	    	change_vol = 7;
	    else if(change_vol <= 0)
	    	change_vol = 0;
		SetDriverVol((unsigned char)change_vol);
		end_vol = change_vol;
		for(i=0; i<4; i++)
		{
            GetRawData(diffdata);
		} 
    }

    FTS_DBG("iTxNum %d %d \n", iTxNum, iRxNum);
	
    for(i = 0;i < iTxNum;i++)
    {
        for(j = 0;j < iRxNum;j++)
        {
            diffdata[i][j] = diffdata[i][j]	- rawdata[i][j] + (bs_diffdata[i][j] * (m_TxOffset[i] + m_RxOffset[j] + 3) * (start_vol - end_vol)) / ((16 + start_vol) * change_rxoffset[j]);

            if(0 == g_testparam.valid_node[i][j])
            {
                continue;
            }

            min_value = g_testparam.diffdata_min_node[i][j];
            max_value = g_testparam.diffdata_max_node[i][j];

            //focal_abs

            if (focal_abs(diffdata[i][j]) < min_value || focal_abs(diffdata[i][j]) > max_value) 
		{
    			bRawdataTest = false;
			//if(i == 0)
			//{
				FTS_DBG("diffdata test failure. min_value=%d max_value=%d diffdata[%d][%d]=%d ", \
					min_value, max_value, i+1, j+1, diffdata[i][j]);
			//}
		}
        }
    }
}
