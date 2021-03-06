/*
    File: gcweak.cc
*/

/*
Copyright (c) 2014, Christian E. Schafmeister
 
CLASP is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.
 
See directory 'clasp/licenses' for full details.
 
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/* -^- */
#include "core/foundation.h"
#include "gcweak.h"
#include "core/object.h"
#ifdef USE_MPS
#include "mps/code/mps.h"
#endif

namespace gctools {


#if 0 //def USE_MPS
    void call_with_alloc_lock( fn_type fn, void* client_data)
    {
        fn(client_data);
    }
#endif

#if 0
    /*! Does not need safeRun */
    uint WeakHashTable::sxhashKey(const value_type& key
#ifdef USE_MPS
                                  , mps_ld_s* locationDependencyP
#endif
        )
    {
        core::HashGenerator hg;
#ifdef USE_MPS
        if ( locationDependencyP && key.pointerp()) {
            mps_ld_add(locationDependencyP
                       ,gctools::_global_arena
                       ,key.base_ref().px_ref() );
        }
#endif
        hg.addPart(reinterpret_cast<uintptr_t>(key.base_ref().px_ref()));
        return hg.hash();
    }
#endif


#if 0
    int WeakHashTable::trySet(core::T_sp tkey, core::T_sp value)
    {
        size_t b;
        if ( tkey == value ) { value = gctools::smart_ptr<core::T_O>(gctools::tagged_ptr<core::T_O>::tagged_sameAsKey); };
        value_type key(tkey);
        int result = WeakHashTable::find(this->_Keys,key
#if USE_MPS
                                         , &this->_LocationDependency
#endif
                                         ,b);
        if (!result) { // this->find(key, this->_HashTable.Keys, true, b)) {
            printf("%s:%d find returned 0\n", __FILE__, __LINE__ );
            return 0;
        }
        if ((*this->_Keys)[b].unboundp()) {
            this->_Keys->set(b,key);
            (*this->_Keys).setUsed((*this->_Keys).used()+1);
            printf("%s:%d key was unboundp at %lu  used = %d\n", __FILE__, __LINE__, b, this->_Keys->used() );
        } else if ((*this->_Keys)[b].deletedp()) {
            this->_Keys->set(b,key);
            GCTOOLS_ASSERT((*this->_Keys).deleted() > 0 );
            (*this->_Keys).setDeleted((*this->_Keys).deleted()-1);
            printf("%s:%d key was deletedp at %lu  deleted = %d\n", __FILE__, __LINE__, b, (*this->_Keys).deleted() );
        }
        (*this->_Values).set(b,value_type(value));
        return 1;
    }
#endif


    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
    //
    // Use safeRun from here on down
    //
    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
#if 0
    int WeakHashTable::rehash(size_t newLength, const value_type& key, size_t& key_bucket)
    {
        int result;
        safeRun<void()>( [&result,this,newLength,&key,&key_bucket] ()->void {
                GCWEAK_LOG(BF("entered rehash newLength = %d") % newLength );
                size_t i, length;
                // buckets_t new_keys, new_values;
                int result = 0;
                length = this->_Keys->length();
                MyType newHashTable(newLength);
                //new_keys = make_buckets(newLength, this->key_ap);
                //new_values = make_buckets(newLength, this->value_ap);
                //new_keys->dependent = new_values;
                //new_values->dependent = new_keys;
#ifdef USE_MPS
                mps_ld_reset(&this->_LocationDependency,gctools::_global_arena);
#endif
                for (i = 0; i < length; ++i) {
                    value_type& old_key = (*this->_Keys)[i];
                    if (!old_key.unboundp() && !old_key.deletedp()) {
                        int found;
                        size_t b;
#ifdef USE_MPS
                        found = WeakHashTable::find(newHashTable._Keys, old_key, &this->_LocationDependency, b);
#else
                        found = WeakHashTable::find(newHashTable._Keys, old_key, b);
#endif
                        GCTOOLS_ASSERT(found);// assert(found);            /* new table shouldn't be full */
                        GCTOOLS_ASSERT((*this->_Keys)[b].unboundp()); /* shouldn't be in new table */
                        newHashTable._Keys->set(b,old_key);
                        (*newHashTable._Values)[b] = (*this->_Values)[i];
                        if (!key.NULLp() && old_key == key ) {
                            key_bucket = b;
                            result = 1;
                        }
                        (*newHashTable._Keys).setUsed((*newHashTable._Keys).used()+1); // TAG_COUNT(UNTAG_COUNT(new_keys->used) + 1);
                    }
                }
                GCTOOLS_ASSERT((*newHashTable._Keys).used() == this->tableSize() );
                // assert(UNTAG_COUNT(new_keys->used) == table_size(tbl));
                this->swap(newHashTable);
            } );
        return result;
    }
#endif


#if 0
/* %%MPS: If we fail to find 'key' in the table, and if mps_ld_isstale
 * returns true, then some of the keys in the table might have been
 * moved by the garbage collector: in this case we need to re-hash the
 * table. See topic/location.
 * Return (values value t) or (values nil nil)
 */
    core::T_mv WeakHashTable::htgethash(core::T_sp tkey, core::T_sp defaultValue)
    {
        core::T_mv result_mv;
        safeRun<void()>( [&result_mv,this,tkey,defaultValue] ()->void
            {
                value_type key(tkey);
                size_t pos;
                int result = gctools::WeakHashTable::find(this->_Keys,key
#ifdef USE_MPS
                                                          ,NULL
#endif
                                                          ,pos);
                if (result) { // WeakHashTable::find(this->_Keys,key,false,pos)) { //buckets_find(tbl, this->keys, key, NULL, &b)) {
                    value_type& k = (*this->_Keys)[pos];
                    GCWEAK_LOG(BF("gethash find successful pos = %d  k= %p k.unboundp()=%d k.base_ref().deletedp()=%d k.NULLp()=%d") % pos % k.pointer() % k.unboundp() % k.base_ref().deletedp() % k.NULLp() );
                    if ( !k.unboundp() && !k.deletedp() ) {
                        GCWEAK_LOG(BF("Returning success!"));
                        core::T_sp value = (*this->_Values)[pos].backcast();
                        if ( value.sameAsKeyP() ) {
                            value = k.backcast();
                        }
                        result_mv = Values(value,_lisp->_true());
                        return;
                    }
                    GCWEAK_LOG(BF("Falling through"));
                }
#ifdef USE_MPS
                if (key.pointerp() && mps_ld_isstale(&this->_LocationDependency, gctools::_global_arena, key.pointer() )) {
                    if (this->rehash( this->_Keys->length(), key, &pos)) {
                        T_sp value = (*this->_Values)[pos].backcast();
                        if ( value.sameAsKeyP() ) {
                            value = key.backcast();
                        }
                        result_mv = Values(value,_lisp->_true());
                        return
                            }
                }
#endif
                result_mv = Values(defaultValue,_Nil<core::T_O>());
                return;
            } );
        return result_mv;
    }
#endif

#if 0
    void WeakHashTable::set( core::T_sp key, core::T_sp value )
    {
        safeRun<void()>(
            [key,value,this] ()->void
            {
                if (this->fullp() || !this->trySet(key,value) ) {
                    int res;
                    value_type dummyKey;
                    size_t dummyPos;
                    this->rehash( (*this->_Keys).length() * 2, dummyKey, dummyPos );
                    core::T_sp tvalue = value;
                    if ( key == value ) {
                        tvalue = gctools::smart_ptr<core::T_O>(gctools::tagged_ptr<core::T_O>::tagged_sameAsKey);
                    }
                    res = this->trySet( key, tvalue);
                    ASSERT(res);
                }
            });
    }
#endif

#if 0
    void WeakHashTable::remhash( core::T_sp tkey )
    {
        safeRun<void()>( [this,tkey] ()->void
            {
                size_t b;
                value_type key(tkey);
#ifdef USE_MPS
                int result = gctools::WeakHashTable::find(this->_Keys, key, NULL, b);
#endif
#ifdef USE_BOEHM
                int result = gctools::WeakHashTable::find(this->_Keys, key, b);
#endif
                if( ! result ||
                    (*this->_Keys)[b].unboundp() ||
                    (*this->_Keys)[b].deletedp() )
                {
#ifdef USE_MPS
                    if(key.pointerp() && !mps_ld_isstale(&this->_LocationDependency, gctools::_global_arena, key.pointer()))
                        return;
#endif
                    if(!this->rehash( (*this->_Keys).length(), key, b))
                        return;
                }
                if( !(*this->_Keys)[b].unboundp() &&
                    !(*this->_Keys)[b].deletedp() )
                {
#ifdef USE_BOEHM
                    IMPLEMENT_MEF(BF("Remove disappearing link to this entry"));
#endif
                    this->_Keys->set(b, value_type(value_type::deleted)); //[b] = value_type(gctools::tagged_ptr<T_O>::tagged_deleted);
                    (*this->_Keys).setDeleted((*this->_Keys).deleted()+1);
                    (*this->_Values)[b] = value_type(gctools::tagged_ptr<core::T_O>::tagged_unbound);
                }
            } );
    }
#endif

#if 0
    void WeakHashTable::clrhash()
    {
        safeRun<void()>( [this] ()->void
            {
                size_t len = (*this->_Keys).length();
                for ( size_t i(0); i<len; ++i ) {
#ifdef USE_BOEHM
                    IMPLEMENT_MEF(BF("Remove disappearing links to this entry"));
#endif
                    this->_Keys->set(i,value_type(gctools::tagged_ptr<core::T_O>::tagged_unbound));
                    (*this->_Values)[i] = value_type(gctools::tagged_ptr<core::T_O>::tagged_unbound);
                }
                (*this->_Keys).setUsed(0);
                (*this->_Keys).setDeleted(0);
#ifdef USE_MPS
                mps_ld_reset(&this->_LocationDependency,gctools::_global_arena);
#endif
            } );
    };
#endif


};

#ifdef USE_MPS
extern "C" {
    using namespace gctools;
    mps_res_t weak_obj_scan(mps_ss_t ss, mps_addr_t base, mps_addr_t limit)
    {
        MPS_SCAN_BEGIN(ss) {
            while (base < limit) {
                WeakObject* weakObj = reinterpret_cast<WeakObject*>(base);
                switch (weakObj->kind()) {
                case WeakBucketKind:
                {      
                    WeakBucketsObjectType* obj = reinterpret_cast<WeakBucketsObjectType*>(weakObj);
                    MPS_FIX12(ss,reinterpret_cast<mps_addr_t*>(&obj->dependent));
                    for ( int i(0), iEnd(obj->length()); i<iEnd; ++i ) {
                        mps_addr_t p = reinterpret_cast<mps_addr_t>(obj->bucket[i].base_ref().px_ref());
                        if (MPS_FIX1(ss,p)) {
                            mps_res_t res = MPS_FIX2(ss,&p);
                            if ( res != MPS_RES_OK ) return res;
                            if ( p == NULL && obj->dependent ) {
                                obj->dependent->bucket[i] = WeakBucketsObjectType::value_type(gctools::tagged_ptr<core::T_O>::tagged_deleted);
                                obj->bucket[i] = WeakBucketsObjectType::value_type(gctools::tagged_ptr<core::T_O>::tagged_deleted);
                            } else {
                                obj->bucket[i].base_ref().px_ref() = reinterpret_cast<gctools::Header_s*>(p);
                            }
                        }
                    }
                    base = (char*)base + sizeof(WeakBucketsObjectType)+sizeof(typename WeakBucketsObjectType::value_type)*obj->length();
                }
                break;
                case StrongBucketKind:
                {      
                    StrongBucketsObjectType* obj = reinterpret_cast<StrongBucketsObjectType*>(base);
                    MPS_FIX12(ss,reinterpret_cast<mps_addr_t*>(&obj->dependent));
                    for ( int i(0), iEnd(obj->length()); i<iEnd; ++i ) {
                        MPS_FIX12(ss,reinterpret_cast<mps_addr_t*>(&(obj->bucket[i].base_ref().px_ref())));
                    }
                    base = (char*)base + sizeof(StrongBucketsObjectType)+sizeof(typename StrongBucketsObjectType::value_type)*obj->length();
                }
                case WeakMappingKind:
                {      
                    WeakMappingObjectType* obj = reinterpret_cast<WeakMappingObjectType*>(weakObj);
                    MPS_FIX12(ss,reinterpret_cast<mps_addr_t*>(&obj->dependent));
                    mps_addr_t p = reinterpret_cast<mps_addr_t>(obj->bucket.base_ref().px_ref());
                    if (MPS_FIX1(ss,p)) {
                        mps_res_t res = MPS_FIX2(ss,&p);
                        if ( res != MPS_RES_OK ) return res;
                        if ( p == NULL && obj->dependent ) {
                            obj->dependent->bucket = WeakBucketsObjectType::value_type(gctools::tagged_ptr<core::T_O>::tagged_deleted);
                            obj->bucket = WeakBucketsObjectType::value_type(gctools::tagged_ptr<core::T_O>::tagged_deleted);
                        } else {
                            obj->bucket.base_ref().px_ref() = reinterpret_cast<gctools::Header_s*>(p);
                        }
                    }
                    base = (char*)base + sizeof(WeakMappingObjectType);
                }
                break;
                case StrongMappingKind:
                {      
                    StrongMappingObjectType* obj = reinterpret_cast<StrongMappingObjectType*>(base);
                    MPS_FIX12(ss,reinterpret_cast<mps_addr_t*>(&obj->dependent));
                    MPS_FIX12(ss,reinterpret_cast<mps_addr_t*>(&(obj->bucket.base_ref().px_ref())));
                    base = (char*)base + sizeof(StrongMappingObjectType);
                }
                break;
                case WeakPointerKind:
                {      
                    WeakPointer* obj = reinterpret_cast<WeakPointer*>(base);
                    MPS_FIX12(ss,reinterpret_cast<mps_addr_t*>(&(obj->value.base_ref().px_ref())));
                    base = (char*)base + sizeof(WeakPointer);
                }
                break;
                default:
                    THROW_HARD_ERROR(BF("Handle other weak kind %d") % weakObj->kind());
                }
            };
        } MPS_SCAN_END(ss);
        return MPS_RES_OK;
    }



    mps_addr_t weak_obj_skip(mps_addr_t base)
    {
        WeakObject* weakObj = reinterpret_cast<WeakObject*>(base);
        switch (weakObj->kind()) {
        case WeakBucketKind:
        {      
            WeakBucketsObjectType* obj = reinterpret_cast<WeakBucketsObjectType*>(weakObj);
            base = (char*)base + sizeof(WeakBucketsObjectType)+sizeof(typename WeakBucketsObjectType::value_type)*obj->length();
        }
        break;
        case StrongBucketKind:
        {      
            StrongBucketsObjectType* obj = reinterpret_cast<StrongBucketsObjectType*>(base);
            base = (char*)base + sizeof(StrongBucketsObjectType)+sizeof(typename StrongBucketsObjectType::value_type)*obj->length();
        }
        case WeakMappingKind:
        {      
            base = (char*)base + sizeof(WeakMappingObjectType);
        }
        break;
        case StrongMappingKind:
        {      
            base = (char*)base + sizeof(StrongMappingObjectType);
        }
        break;
        case WeakPointerKind:
        {      
            base = (char*)base + sizeof(WeakPointer);
        }
        break;
        case WeakFwdKind:
        {      
            weak_fwd_s* obj = reinterpret_cast<weak_fwd_s*>(base);
            base = (char*)base + Align(obj->size.fixnum());
        }
        break;
        case WeakFwd2Kind:
        {      
            base = (char*)base + Align(sizeof(weak_fwd2_s));
        }
        break;
        case WeakPadKind:
        {      
            weak_pad_s* obj = reinterpret_cast<weak_pad_s*>(base);
            base = (char*)base + Align(obj->size.fixnum());
        }
        break;
        case WeakPad1Kind:
        {      
            base = (char*)base + Align(sizeof(weak_pad1_s));
        }
        default:
            THROW_HARD_ERROR(BF("Handle weak_obj_skip other weak kind %d") % weakObj->kind());
        }
        return base;
    };


    void weak_obj_fwd(mps_addr_t old, mps_addr_t newv)
    {
        WeakObject* weakObj = reinterpret_cast<WeakObject*>(old);
        mps_addr_t limit = weak_obj_skip(old);
        size_t size = (char *)limit - (char *)old;
        assert(size >= Align(sizeof(weak_fwd2_s)));
        if (size == Align(sizeof(weak_fwd2_s))) {
            weak_fwd2_s* weak_fwd2_obj = reinterpret_cast<weak_fwd2_s*>(weakObj);
            weak_fwd2_obj->setKind(WeakFwd2Kind);
            weak_fwd2_obj->fwd = reinterpret_cast<WeakObject*>(newv);
        } else {
            weak_fwd_s* weak_fwd_obj = reinterpret_cast<weak_fwd_s*>(weakObj);
            weak_fwd_obj->setKind(WeakFwdKind);
            weak_fwd_obj->fwd = reinterpret_cast<WeakObject*>(newv);
            weak_fwd_obj->size = gctools::tagged_ptr<gctools::Fixnum_ty>(size);
        }
    }


    mps_addr_t weak_obj_isfwd(mps_addr_t addr)
    {
        WeakObject* obj = reinterpret_cast<WeakObject*>(addr);
        switch (obj->kind()) {
        case WeakFwd2Kind:
        {
            weak_fwd2_s* weak_fwd2_obj = reinterpret_cast<weak_fwd2_s*>(obj);
            return weak_fwd2_obj->fwd;
        }
        case WeakFwdKind:
        {
            weak_fwd_s* weak_fwd_obj = reinterpret_cast<weak_fwd_s*>(obj);
            return weak_fwd_obj->fwd;
        }
        }
        return NULL;
    }



    void weak_obj_pad(mps_addr_t addr, size_t size)
    {
        WeakObject* weakObj = reinterpret_cast<WeakObject*>(addr);
        assert(size >= Align(sizeof(weak_pad1_s)));
        if (size == Align(sizeof(weak_pad1_s))) {
            weakObj->setKind(WeakPad1Kind);
        } else {
            weakObj->setKind(WeakPadKind);
            weak_pad_s* weak_pad_obj = reinterpret_cast<weak_pad_s*>(addr);
            weak_pad_obj->size = gctools::tagged_ptr<gctools::Fixnum_ty>(size);
        }
    }







};
#endif
