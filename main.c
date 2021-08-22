#include <stdio.h>
#include <stdlib.h>

#pragma pack(1)

typedef struct Fat12_header
{
	unsigned char com1[3];
	unsigned char oemName[8];
	unsigned short bytesPerSec;
	unsigned char secPerClus;
	unsigned short mbrSrcs;
	unsigned char fatNum;
	unsigned short rootCnt;
	unsigned short totalSec16;
	unsigned char media;
	unsigned short fatPerSec;
	unsigned short secPerTrk;
	unsigned short headNum;
	unsigned int hidSecNum;
	unsigned int totalSec32;
	unsigned char driveNum;
	unsigned char reverse;
	unsigned char bootsig;
	unsigned int volid;
	unsigned char volName[11];
	unsigned char fileSysType[8];
	unsigned char com2[448];
	unsigned short bootFlag;
} Fat12_header;

typedef struct Fat12_clus
{
	unsigned char data[3];
} Fat12_clus;

typedef struct Fat12_table
{
	Fat12_clus data[1536];
} Fat12_table;

typedef struct Root_table
{
	unsigned char baseFileName[8];
	unsigned char exFileName[3];
	unsigned char fileAttr;
	unsigned char reverse[10];
	unsigned short time;
	unsigned short date;
	unsigned short clus;
	unsigned int fileSize;
} Root_table;

typedef struct Fat12_message
{
	Fat12_header header;
	Fat12_table fatTables[2];
} Fat12_message;

unsigned short getClus(unsigned char *buffer, char flag)
{
	unsigned short ans = 0;
	if (!flag)
	{
		ans += buffer[0];
		ans += (buffer[1] << 8) & 0xfff;
	}
	else
	{
		ans += buffer[1] << 4;
		ans += buffer[0] >> 4;
	}
	return ans;
}

int cmpStr(unsigned char *src, char *dest)
{
	while (*dest != 0)
	{
		if (*src != *dest)
		{
			return 0;
		}
		src++;
		dest++;
	}
	return 1;
}

void parse(char *fileName, char *base, char *ex)
{
	while (*fileName && *fileName != '.')
	{
		*base = *fileName;
		if (*base >= 'a' && *base <= 'z')
		{
			*base -= ('a' - 'A');
		}
		base++;
		fileName++;
	}
	*base = 0;
	fileName++;
	while (*fileName)
	{
		*ex = *fileName;
		if (*ex >= 'a' && *ex <= 'z')
		{
			*ex -= ('a' - 'A');
		}
		ex++;
		fileName++;
	}
	*ex = 0;
}

int main(int argc, char **argv)
{
	if (argc == 2)
	{
		FILE *fp = fopen("disk.img", "rb");
		char baseName[9], exName[5];
		parse(argv[1], baseName, exName);
		if (fp != NULL)
		{
			Fat12_message message;
			fread(&message, sizeof(message), 1, fp);
			Root_table *rootTables = (Root_table *) malloc(sizeof(Root_table) * message.header.rootCnt);
			if (rootTables != NULL)
			{
				fread(rootTables, sizeof(rootTables), message.header.rootCnt, fp);
				for (int i = 0; i < message.header.rootCnt; i++)
				{
					if (cmpStr(rootTables[i].baseFileName, baseName) && cmpStr(rootTables[i].exFileName, exName))
					{
						int clus = rootTables[i].clus;
						while (clus != 0xFF0 && clus != 0xFFF)
						{
							char *buffer = (char *) calloc(sizeof(char), message.header.secPerClus * 512);
							fseek(fp, 33 * 512, SEEK_SET);
							fseek(fp, (clus - 2) * message.header.secPerClus * 512, SEEK_CUR);
							fread(buffer, message.header.secPerClus * 512, 1, fp);
							puts(buffer);
							free(buffer);
							clus = getClus(&message.fatTables[0].data[clus / 2].data[clus % 2], clus % 2);
						}
						break;
					}
				}
				free(rootTables);
			}
			else
			{
				perror("Cannot alloc memory");
			}
			fclose(fp);
		}
		else
		{
			perror("Cannot open file disk.img");
		}
	}
	else
	{
		fprintf(stderr, "Argument wrong!\n");
	}
	return 0;
}
