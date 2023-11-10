#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<linux/nvme_ioctl.h>
#include<sys/ioctl.h>
#include<fcntl.h>
#include<time.h>
#include<string.h>
#include<linux/types.h>
#include<pthread.h>
#include <linux/fs.h>

#define IDENTIFY_LEN 0x1000

int sector_size = 512;
int ns= 1; /*namespace id*/
__u64 seq_lba ;/*this keeps the sequential LBA*/
char file_name[128];/*it holds the device file name*/
__u64 lba_max;/*to be filled by identify function*/

int con_identify()
{
	//printf("\ncon_identify\n");
	int fd = open(file_name,O_RDWR);
	if(fd == 0){
		printf("could not open device file\n");
		return 1;
	}
	//else printf("device file opened successfully\n");

	char data[IDENTIFY_LEN];
	for(register int i=0; i<IDENTIFY_LEN;data[i++]=0);
	struct nvme_admin_cmd cmd;
	cmd.opcode = 0x06;
	cmd.nsid = 0;
	cmd.addr = reinterpret_cast<__u64>(data);
	cmd.data_len = IDENTIFY_LEN;
	cmd.cdw10 = 1;

	int ret = ioctl(fd,NVME_IOCTL_ADMIN_CMD,&cmd);
	if (ret)
	{
		printf("namespace identify failed %d\n", ret);
		close(fd);
		return 1;
	}
	//if(ret==0) printf("namespace identify successful \n");
	//else printf("namespace identify failed %d\n",ret);

	//printf("CONTROLLER IDENTIFY DETAILS:\n");
////	for(register int i=0;i<IDENTIFY_LEN;printf("%c",data[i++]));
	//printf("IDENTIFY DETAILS\n");
////	printf("PCI Vendor ID: %c%c\n", data[0],data[1]);
////	printf("PCI subsystem vendor ID: %c%c\n",data[2],data[3]);
	printf("Serial Number: ");
	for(int i =0; i<20; printf("%c",data[(i++)+4]));
	printf("\n");
	printf("Model Number: ");
	for(int i=0; i<40; printf("%c",data[(i++)+24]) );
	printf("\n");
	printf("Firmware Revision: ");
	for(int i = 0;i<8; printf("%c",data[(i++)+64]));
	printf("\n");
	//printf("Maximum data transfer size 2^(in units of CAP.MPSMIN): %d\n", (int)data[77] );
	//printf("Submission Queue Entry Size 2^:\n");
	//printf("--maximum: %d\t", (int)(data[512]>>4));
	//printf("--required: %d\n",(int)(data[512]&0x0f));
	//printf("Completion Queue Entry Size 2^:\n");
	//printf("--maximum: %d\t", (int)(data[513]>>4));
	//printf("--required: %d\n",(int)(data[513]&0x0f));
	//printf("Number of Namespaces: %d\n", *( (int* )(data+516)));
	//if(data[525]&0x1)
	//	printf("Volatile Write Cache present\n");	
	//printf("Number of Power States supported: %d\n",(int)data[263]);
	
	close(fd);
	return 0;
}

int ns_identify()
{
//	printf("\nns_identify\n");
	int fd = open(file_name,O_RDWR);
	if(fd == 0){
		printf("could not open device file\n");
		return 1;
	}
	//else 
	//	printf("device file opened successfully\n");

	char data[IDENTIFY_LEN];
	for(register int i=0; i<IDENTIFY_LEN;data[i++]=0);
	struct nvme_admin_cmd cmd;
	cmd.opcode = 0x06;
	cmd.nsid = 1;
	cmd.addr = reinterpret_cast<__u64>(data);
	cmd.data_len = IDENTIFY_LEN;
	cmd.cdw10 = 0;

	int ret = ioctl(fd, NVME_IOCTL_ADMIN_CMD, &cmd);
	if(ret) 
	{ 
		printf("NVME_IOCTL_ADMIN_CMD failed %d\n", ret);
		close(fd);
		return 1;
	}
	//else
	//	printf("NVME_IOCTL_ADMIN_CMD successful \n");

	//printf("NAMESPACE SIZE(MAX LBA): %lld\n",*((__u64*)data));
	//printf("NAMESPACE CAPACITY(ALLOWED LBA - THIN OVERPROVISIONING): %lld\n",*((__u64*)(data+8)));
	//printf("NAMESPACE UTILIZATION: %lld\n",*((__u64*)(data+16)));
	////if(data[24]&0x1)
	////	printf("Namespace supports Thin OverProvisioning(NAMESPZE CAPACITY reported may be less than SIZE)\n");
	////else printf("Thin Overprovisioning not supported\n");
	//printf("NUMBER OF LBA FORMATS: %d, FORMATED LBA SIZE: %d\n",(__u8)data[25], (__u8)data[26]);
	////printf("FORMATED LBA SIZE: %d\n",(__u8)data[26]);
	//printf("LBA FORMATE 0: METADAT SIZE: %d, LBA DATA SIZE(2^n): %d\n", *((__u16*)(data+128)), *((__u8*)(data + 130)));
	////printf("LBA FORMATE 0: LBA DATA SIZE(2^n): %d\n", *((__u8*)(data+130)));

	printf("%lld sectors\n", *((__u64*)(data + 8)));

	sector_size = *((__u8*)(data+130));
	sector_size = (1<<sector_size);
	lba_max = *((__u64*)data);

	close(fd);
	return 0;
}

int nvme_info(char* drvname)
{
	printf("Device %s :\n", drvname);
	//printf("\nnvme_info\n");
	int temp;
	//strcpy(file_name, "/dev/nvme0n1");
	strcpy(file_name, drvname);

	temp = con_identify();
	if (temp > 0) { printf("Con Identify Failed\n"); return -1; }
	temp = ns_identify();
	if (temp > 0) { printf("NS Identify Failed\n"); return -1; }
	lba_max = lba_max - 2;
	//printf("lba max: %lld \t sector size: %d\n", lba_max, sector_size);

	int sysflag = O_RDWR | O_DIRECT | O_SYNC;
	int hfile = open(drvname, sysflag, 0666);
	if (hfile < 0)
	{
		printf("ERROR: can not open %s\n", drvname);
		return -1;
	}

	unsigned long long numblocks = 0;
	ioctl(hfile, BLKGETSIZE64, &numblocks);
	printf("%.2f GiB, %llu bytes\n", (double)numblocks / (1024 * 1024 * 1024), numblocks);
	close(hfile);
	return 0;
}




