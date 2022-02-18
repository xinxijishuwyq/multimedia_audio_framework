#include<OpenSLES.h>
#include<OpenSLES_Platform.h>

/*---------------------------------------------------------------------------*/
/* OH Buffer Queue Interface                                                    */
/*---------------------------------------------------------------------------*/

extern const SLInterfaceID SL_IID_OH_BUFFERQUEUE;

struct SLOHBufferQueueItf_;
typedef const struct SLOHBufferQueueItf_ * const * SLOHBufferQueueItf;

typedef void (SLAPIENTRY *SlOHBufferQueueCallback)(
	SLOHBufferQueueItf caller,
	void *pContext,
    SLuint32 size
);

/** OH Buffer queue state **/

typedef struct SLOHBufferQueueState_ {
	SLuint32	count;
	SLuint32	playIndex;
} SLOHBufferQueueState;


struct SLOHBufferQueueItf_ {
	SLresult (*Enqueue) (
		SLOHBufferQueueItf self,
		const void *pBuffer,
		SLuint32 size
	);
	SLresult (*Clear) (
		SLOHBufferQueueItf self
	);
	SLresult (*GetState) (
		SLOHBufferQueueItf self,
		SLOHBufferQueueState *pState
	);
	SLresult (*GetBuffer) (
		SLOHBufferQueueItf self,
		SLuint8** pBuffer,
		SLuint32& pSize
	);
	SLresult (*RegisterCallback) (
		SLOHBufferQueueItf self,
		SlOHBufferQueueCallback callback,
		void* pContext
	);
};