
#include "tinyos.h"
#include "kernel_pipe.h"
#include "kernel_sched.h"


int nothing(void* this, char *buf, unsigned int size){
	return -1;
}

int nothingConst(void* this, const char *buf, unsigned int size){
	return -1;
}

int pipe_read(void* this, char *buf, unsigned int size)
{
	pipe_CB* pipeCB = (pipe_CB*) this;

	//if invalid pipe_cb return -1
	if(pipeCB == NULL){
		return -1;
	}

	//if reader is closed return -1
	if(pipeCB->pit.read == NOFILE){
		return -1;
	}

	int count = 0;

	//an o readPTR ftasei ton writePTR kane sleep sto hasData
	while((pipeCB->readPTR + 1)%BUF_SIZE == pipeCB->writePTR && pipeCB->pit.write != NOFILE){
		kernel_wait(&(pipeCB->hasData), SCHED_PIPE);
	}

	//an o writer exei kanei close diabase mexri telous

	//as diabasoume to solinaki
	while(count < size && count != BUF_SIZE){
		//an omos o reader paei na diabasi kati pou den exei grafei, ksou
		if((pipeCB->readPTR + 1)%BUF_SIZE == pipeCB->writePTR)
			break;
		buf[count] = pipeCB->buffer[pipeCB->readPTR];
		pipeCB->buffer[pipeCB->readPTR] = '\0';
		pipeCB->readPTR = (pipeCB->readPTR + 1) % BUF_SIZE;
		count++;
	}
	kernel_broadcast(&(pipeCB->hasSpace));
	return count;
}

int pipe_write(void* this, const char* buf, unsigned int size)
{
	pipe_CB* pipeCB = (pipe_CB*) this;

	//if invalid pipe_cb return -1
	if(pipeCB == NULL){
		return -1;
	}

	//if writer or reader is closed return -1
	if(pipeCB->pit.write == NOFILE || pipeCB->pit.read == NOFILE ){
		return -1;
	}

	int count = 0;

	//an o writePTR ftasei ton readPTR kane sleep sto hasSpace
	while((pipeCB->writePTR + 1)%BUF_SIZE == pipeCB->readPTR){
		kernel_wait(&(pipeCB->hasSpace), SCHED_PIPE);
	}

	//as grapsoume sto solinaki
	while(count < size && count != BUF_SIZE){
		//an omos o writer paei na grapsei kati pou den diabasame ksou
		if((pipeCB->writePTR + 1)%BUF_SIZE == pipeCB->readPTR)
			break;

		/*bale ston buffer tou pipe stin thesi tou wrPTR 
		tin thesi counter tou buf tou orismatos */
		pipeCB->buffer[pipeCB->writePTR] = buf[count];
		//auksise ton writePTR ston bounded buffer
		pipeCB->writePTR = (pipeCB->writePTR + 1) % BUF_SIZE;
		//auksise ton counter
		count++;
	}
	//pes kana koimismeno pos egrapses
	kernel_broadcast(&(pipeCB->hasData));
	return count;
}

int reader_Close(void* streamobj)
{
//kane pipeCB->pit.read = NOFILE
	return -1;
}

int writer_Close(void* streamobj)
{
//kane pipeCB->pit.write = NOFILE
	return -1;
}

int sys_Pipe(pipe_t* pipe)
{
	//topikes metablites gia fid fcb kai pipeControlBlock
	Fid_t fid[2];
	FCB* fcb[2];
	pipe_CB* cb = (pipe_CB*) xmalloc(sizeof(pipe_CB));

	//kane reserve 2 FCBs
	int reservedFCB = FCB_reserve(2,fid,fcb);
	//an apotuxei to reserve return -1	
	if(!reservedFCB){
		return -1;	
	}
	/*kane tous reader kai writer tou pipe_t na 
	deixnoun ta fid pou kaname reserve*/
	pipe->read = fid[0];
	pipe->write = fid[1];

	/*arxikopoiisi ton periexomenon tou pipeCB*/
	cb->pit = *pipe;
	cb->readPTR = 0;
	cb->writePTR = 0;
	cb->hasSpace = COND_INIT;
	cb->hasData = COND_INIT;

	/*streamobj kai func gia ta fcb*/
	fcb[0]->streamobj = cb;
	fcb[1]->streamobj = cb;
	fcb[0]->streamfunc = &reader_ops;
	fcb[1]->streamfunc = &writer_ops;
	//ola kalws
	return 0;
}

/*
I proti douleia tis pipe einai na kanei ena
FCB_reserve(Z,Fid_t *, FCB**)
na dimiourgisoume diladi to path ap to pcb
mexri to file table.
Theloume ena fidT gia read ki ena gia write.
An ola kalos epistrefei 0 allios -1.
Episis prepei na dimiourgoume ena pipe_CB to
opoio tha exei mesa enan buffer(char*)
SIMEIOSI o buffer einai mia kukliki lista (bounded buffer).
ki ena BUF_SIZE=8192 kai FCB* r,w kai 
uint readPTR,writePTR kai
CondVar hasSpace, hasData.
o reader koimatai pano sto hasData kai o writer pano stin hasSpace.

Episis edo ulopoiisi ton pipe_read(pipe_CB,buf,size) kai 
pipe_write(pipe_CB,buf,size) kathos kai
close_reader kai close_writer.
*/
