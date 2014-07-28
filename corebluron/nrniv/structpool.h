/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef structpool_h
#define structpool_h

// same as ../nrncvode/pool.h but items do not require a clear method.

// create and manage a vector of objects as a memory pool of those objects
// the object must have a void clear() method which takes care of any 
// data the object contains which should be deleted upon free_all.
// clear() is NOT called on free, only on free_all.

// the chain of Pool
// is only for extra items in a pool_ and no other fields are used.
// the pool doubles in size every time a chain Pool is added.
// maxget() tells the most number of pool items used at once.

#define declareStructPool(Pool,T) \
class Pool { \
public: \
	Pool(long count); \
	~Pool(); \
	T* alloc(); \
	void hpfree(T*); \
	int maxget() { return maxget_;} \
	void free_all(); \
private: \
	void grow(); \
private: \
	T** items_; \
	T* pool_; \
	long pool_size_; \
	long count_; \
	long get_; \
	long put_; \
	long nget_; \
	long maxget_; \
	Pool* chain_; \
}; \
 \

#define implementStructPool(Pool,T) \
Pool::Pool(long count) { \
	count_ = count; \
	pool_ = new T[count_]; \
	pool_size_ = count; \
	items_ = new T*[count_]; \
	for (long i = 0; i < count_; ++i) items_[i] = pool_ + i; \
	get_ = 0; \
	put_ = 0; \
	nget_ = 0; \
	maxget_ = 0; \
	chain_ = 0; \
} \
 \
void Pool::grow() { \
	assert(get_ == put_); \
	Pool* p = new Pool(count_); \
	p->chain_ = chain_; \
	chain_ = p; \
	long newcnt = 2*count_; \
	T** itms = new T*[newcnt]; \
	long i, j; \
	put_ += count_; \
	for (i = 0; i < get_; ++i) { \
		itms[i] = items_[i]; \
	} \
	for (i = get_, j = 0; j < count_; ++i, ++j) { \
		itms[i] = p->items_[j]; \
	} \
	for (i = put_, j = get_; j < count_; ++i, ++j) { \
		itms[i] = items_[j]; \
	} \
	delete [] items_; \
	delete [] p->items_; \
	p->items_ = 0; \
	items_ = itms; \
	count_ = newcnt; \
} \
 \
Pool::~Pool() { \
	if (chain_) { \
		delete chain_; \
	} \
	delete [] pool_; \
	if (items_) { \
		delete [] items_; \
	} \
} \
 \
T* Pool::alloc() { \
	if (nget_ >= count_) { grow(); } \
	T* item = items_[get_]; \
	get_ = (get_+1)%count_; \
	++nget_; \
	if (nget_ > maxget_) { maxget_ = nget_; } \
	return item; \
} \
 \
void Pool::hpfree(T* item) { \
	assert(nget_ > 0); \
	items_[put_] = item; \
	put_ = (put_ + 1)%count_; \
	--nget_; \
} \
\
void Pool::free_all() { \
	Pool* pp; \
	long i; \
	nget_ = 0; \
	get_ = 0; \
	put_ = 0; \
	for(pp = this; pp; pp = pp->chain_) { \
		for (i=0; i < pp->pool_size_; ++i) { \
			items_[put_++] = pp->pool_ + i; \
		} \
	} \
	assert(put_ == count_); \
	put_ = 0; \
} \
\

#endif
