#include <linux/string.h>
#include <linux/kernel.h>
#include <asm/unistd.h>

#include "ini.h"


char CFG_SSL = '[';  /* ���־��Section Symbol --�ɸ���������Ҫ���ж�����ģ��� { }��*/
char CFG_SSR = ']';  /* ���־��Section Symbol --�ɸ���������Ҫ���ж�����ģ��� { }��*/
char CFG_NIS = ':';  /* name �� index ֮��ķָ��� */
char CFG_NTS = '#';  /* ע�ͷ�*/

char * ini_str_trim_r(char * buf);
char * ini_str_trim_l(char * buf);
static int ini_file_get_line(char *filedata, char *buffer, int maxlen); 
static int ini_split_key_value(char *buf, char **key, char **val); 
static long atol(char *nptr);


/*************************************************************
Function: ���key��ֵ
Input: char * filedata���ļ���char * section����ֵ��char * key����ֵ
Output: char * value��key��ֵ
Return: 0		SUCCESS
		-1		δ�ҵ�section
		-2		δ�ҵ�key
		-10		�ļ���ʧ��
		-12		��ȡ�ļ�ʧ��
		-14		�ļ���ʽ����
		-22		������������С
Note: 
*************************************************************/
int ini_get_key(char *filedata, char * section, char * key, char * value)
{
	char buf1[MAX_CFG_BUF + 1], buf2[MAX_CFG_BUF + 1];
	char *key_ptr, *val_ptr;
	int  n, ret;
	int dataoff = 0;
	
	*value='\0';

	while(1) { /* ������section */
		ret = CFG_ERR_READ_FILE;
		n = ini_file_get_line(filedata+dataoff, buf1, MAX_CFG_BUF);
		dataoff += n;
		if(n < -1)
			goto r_cfg_end;
		ret = CFG_SECTION_NOT_FOUND;
		if(n < 0)
			goto r_cfg_end; /* �ļ�β��δ���� */ 

		if(n > MAX_CFG_BUF)
			goto r_cfg_end;

		n = strlen(ini_str_trim_l(ini_str_trim_r(buf1)));
		if(n == 0 || buf1[0] == CFG_NTS)
			continue;       /* ���� �� ע���� */ 

		ret = CFG_ERR_FILE_FORMAT;
		if(n > 2 && ((buf1[0] == CFG_SSL && buf1[n-1] != CFG_SSR)))
			goto r_cfg_end;
		if(buf1[0] == CFG_SSL) {
			buf1[n-1] = 0x00;
			if(strcmp(buf1+1, section) == 0)
			{
				break; /* �ҵ���section */
			}
		} 
	} 

	while(1){ /* ����key */ 
		ret = CFG_ERR_READ_FILE;
		n = ini_file_get_line(filedata+dataoff, buf1, MAX_CFG_BUF);
		dataoff += n;
		if(n < -1) 
			goto r_cfg_end;
		ret = CFG_KEY_NOT_FOUND;
		if(n < 0)
			goto r_cfg_end;/* �ļ�β��δ����key */ 
		n = strlen(ini_str_trim_l(ini_str_trim_r(buf1)));
		if(n == 0 || buf1[0] == CFG_NTS) 
			continue;       /* ���� �� ע���� */ 
		ret = CFG_KEY_NOT_FOUND; 
		if(buf1[0] == CFG_SSL)
		{
			goto r_cfg_end;
		}
		if(buf1[n-1] == '+') { /* ��+�ű�ʾ��һ�м���  */ 		
			buf1[n-1] = 0x00; 
			while(1) {			
				ret = CFG_ERR_READ_FILE; 
				n = ini_file_get_line(filedata+dataoff, buf2, MAX_CFG_BUF);
				dataoff += n;
				if(n < -1) 
					goto r_cfg_end; 
				if(n < 0) 
					break;/* �ļ����� */ 

				n = strlen(ini_str_trim_r(buf2)); 
				ret = CFG_ERR_EXCEED_BUF_SIZE; 
				if(n > 0 && buf2[n-1] == '+'){/* ��+�ű�ʾ��һ�м��� */ 
				 	buf2[n-1] = 0x00; 
					if( (strlen(buf1) + strlen(buf2)) > MAX_CFG_BUF) 
						goto r_cfg_end; 
					strcat(buf1, buf2); 
					continue; 
				} 
				if(strlen(buf1) + strlen(buf2) > MAX_CFG_BUF) 
					goto r_cfg_end; 
				strcat(buf1, buf2); 
				break; 
			} 
		}
		ret = CFG_ERR_FILE_FORMAT; 
		if(ini_split_key_value(buf1, &key_ptr, &val_ptr) != 1) 
			goto r_cfg_end; 

		ini_str_trim_l(ini_str_trim_r(key_ptr)); 
		if(strcmp(key_ptr, key) != 0) 
			continue;                                  /* ��keyֵ��ƥ�� */ 
		strcpy(value, val_ptr); 
		break; 
	}
	ret = CFG_OK; 
r_cfg_end: 

	return ret; 
} 
/*************************************************************
Function: �������section
Input:  char *filename���ļ�,int max ���ɷ��ص�section�ĸ���
Output: char *sections[]�����section����
Return: ����section�����������������ظ�����
		-10			�ļ��򿪳���
		-12			�ļ���ȡ����
		-14			�ļ���ʽ����
Note: 
*************************************************************/
int ini_get_sections(char *filedata, unsigned char * sections[], int max)
{
	//FILE *fp; 
	char buf1[MAX_CFG_BUF + 1]; 
	int n, n_sections = 0, ret; 
	int dataoff = 0;
	
	while(1) {/*������section */
		ret = CFG_ERR_READ_FILE;
		n = ini_file_get_line(filedata+dataoff, buf1, MAX_CFG_BUF);
		dataoff += n;
		if(n < -1) 
			goto cfg_scts_end; 
		if(n < 0)
			break;/* �ļ�β */ 
		n = strlen(ini_str_trim_l(ini_str_trim_r(buf1)));
		if(n == 0 || buf1[0] == CFG_NTS) 
			continue;       /* ���� �� ע���� */ 
		ret = CFG_ERR_FILE_FORMAT;
		if(n > 2 && ((buf1[0] == CFG_SSL && buf1[n-1] != CFG_SSR)))
			goto cfg_scts_end;
		if(buf1[0] == CFG_SSL) {
			if (max!=0){
				buf1[n-1] = 0x00;
				strcpy((char *)sections[n_sections], buf1+1);
				if (n_sections>=max)
					break;		/* �����ɷ��������� */
			}
			n_sections++;
		} 

	} 
	ret = n_sections;
cfg_scts_end: 
//	if(fp != NULL)
//		fclose(fp);
	return ret;
} 


/*************************************************************
Function: ȥ���ַ����ұߵĿ��ַ�
Input:  char * buf �ַ���ָ��
Output: 
Return: �ַ���ָ��
Note: 
*************************************************************/
char * ini_str_trim_r(char * buf)
{
	int len,i;
	char tmp[512];

	memset(tmp, 0, sizeof(tmp));
	len = strlen(buf);
	
	memset(tmp,0x00,len);
	for(i = 0;i < len;i++) {
		if (buf[i] !=' ')
			break;
	}
	if (i < len) {
		strncpy(tmp,(buf+i),(len-i));
	}
	strncpy(buf,tmp,len);
	return buf;
}

/*************************************************************
Function: ȥ���ַ�����ߵĿ��ַ�
Input:  char * buf �ַ���ָ��
Output: 
Return: �ַ���ָ��
Note: 
*************************************************************/
char * ini_str_trim_l(char * buf)
{
	int len,i;	
	char tmp[512];

	memset(tmp, 0, sizeof(tmp));
	len = strlen(buf);

	memset(tmp,0x00,len);

	for(i = 0;i < len;i++) {
		if (buf[len-i-1] !=' ')
			break;
	}
	if (i < len) {
		strncpy(tmp,buf,len-i);
	}
	strncpy(buf,tmp,len);
	return buf;
}
/*************************************************************
Function: ���ļ��ж�ȡһ��
Input:  FILE *fp �ļ������int maxlen ��������󳤶�
Output: char *buffer һ���ַ���
Return: >0		ʵ�ʶ��ĳ���
		-1		�ļ�����
		-2		���ļ�����
Note: 
*************************************************************/
static int ini_file_get_line(char *filedata, char *buffer, int maxlen)
{
	int  i, j; 
	char ch1; 
	
	for(i=0, j=0; i<maxlen; j++) { 
		ch1 = filedata[j];
		if(ch1 == '\n' || ch1 == 0x00) 
			break; /* ���� */ 
		if(ch1 == '\f' || ch1 == 0x1A) {      /* '\f':��ҳ��Ҳ����Ч�ַ� */ 			
			buffer[i++] = ch1; 
			break; 
		}
		if(ch1 != '\r') buffer[i++] = ch1;    /* ���Իس��� */ 
	} 
	buffer[i] = '\0'; 
	return i+2; 
} 
/*************************************************************
Function: ����key��value
			key=val
			jack   =   liaoyuewang 
			|      |   | 
			k1     k2  i 
Input:  char *buf
Output: char **key, char **val
Return: 1 --- ok 
		0 --- blank line 
		-1 --- no key, "= val" 
		-2 --- only key, no '=' 
Note: 
*************************************************************/
static int  ini_split_key_value(char *buf, char **key, char **val)
{
	int  i, k1, k2, n; 
	
	if((n = strlen((char *)buf)) < 1)
		return 0; 
	for(i = 0; i < n; i++) 
		if(buf[i] != ' ' && buf[i] != '\t')
			break; 

	if(i >= n)
		return 0;

	if(buf[i] == '=')
		return -1;
	
	k1 = i;
	for(i++; i < n; i++) 
		if(buf[i] == '=') 
			break;

	if(i >= n)
		return -2;
	k2 = i;
	
	for(i++; i < n; i++)
		if(buf[i] != ' ' && buf[i] != '\t') 
			break; 

	buf[k2] = '\0'; 

	*key = buf + k1; 
	*val = buf + i; 
	return 1; 
} 

int my_atoi(const char *str)
{
	int result = 0;
	int signal = 1; /* Ĭ��Ϊ���� */
	if((*str>='0'&&*str<='9')||*str=='-'||*str=='+') {
		if(*str=='-'||*str=='+') { 
			if(*str=='-')
				signal = -1; /*���븺��*/
			str++;
		}
	}
	else 
		return 0;
	/*��ʼת��*/
	while(*str>='0' && *str<='9')
	   result = result*10 + (*str++ - '0' );
	
	return signal*result;
}

int isspace(int x)  
{  
    if(x==' '||x=='\t'||x=='\n'||x=='\f'||x=='\b'||x=='\r')  
        return 1;  
    else   
        return 0;  
}  
  
int isdigit(int x)  
{  
    if(x<='9' && x>='0')           
        return 1;   
    else   
        return 0;  
  
} 

static long atol(char *nptr)
{
	int c; /* current char */
	long total; /* current total */
	int sign; /* if ''-'', then negative, otherwise positive */
	/* skip whitespace */
	while ( isspace((int)(unsigned char)*nptr) )
		++nptr;
	c = (int)(unsigned char)*nptr++;
	sign = c; /* save sign indication */
	if (c == '-' || c == '+')
		c = (int)(unsigned char)*nptr++; /* skip sign */
	total = 0;
	while (isdigit(c)) {
		total = 10 * total + (c - '0'); /* accumulate digit */
		c = (int)(unsigned char)*nptr++; /* get next char */
	}
	if (sign == '-')
		return -total;
	else
		return total; /* return result, negated if necessary */
}
/***
*int atoi(char *nptr) - Convert string to long
*
*Purpose:
* Converts ASCII string pointed to by nptr to binary.
* Overflow is not detected. Because of this, we can just use
* atol().
*
*Entry:
* nptr = ptr to string to convert
*
*Exit:
* return int value of the string
*
*Exceptions:
* None - overflow is not detected.
*
*******************************************************************************/
int atoi(char *nptr)
{
	return (int)atol(nptr);
}

