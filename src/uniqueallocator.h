#ifndef _UNIQUEALLOCATOR_H_
#define _UNIQUEALLOCATOR_H_

#include <memory>

// The following template comes from
//     https://howardhinnant.github.io/allocator_boilerplate.html

namespace slidingbuffer {

/*!
* UniqueAllocator replaces the std::allocator for the internal
* stuctures of SlidingBuffer.
* 
* Containers using this allocator can conveniently be used
* to hold pointers to dynamically allocated objects, so
* long as the only access to the objecs is uniquely through
* the container.
* 
* Since the pointers are stored uniquely in their particular
* collection, UniqueAllocator provides a method which deletes
* the referenced object when its pointer is deleted. This combines
* efficiency of access with guaranteed deallocation.
* 
* e.g., SlidingBuffer maintains a container of Segments internally.
* These segments are allocated dynamically, and it is their
* addresses which are stored. In order to maximise efficiency,
* the addresses are stored as raw pointers, but this might lead
* to memory leaks when the pointers are deallocated.
* 
*/

template <typename T> class UniqueAllocator
{
public:
    using value_type    = T;      //!< Type of values to allocate

    // not required, unless used
    /*! Constuctor */
    UniqueAllocator() noexcept {}
    
    template <class U> UniqueAllocator(UniqueAllocator<U> const&) noexcept {}

    // Use pointer if pointer is not a value_type*
    value_type* allocate(std::size_t n)
    {
        return static_cast<value_type*>(::operator new (n*sizeof(value_type)));
    }
    
    // Use pointer if pointer is not a value_type*
    void deallocate(value_type* p, std::size_t n) noexcept
    {
        ::operator delete(p);
    }

    /*! If the argument matches a pointer type,
     *  delete the object at which it points
     * 
     * The assumption is that pointers held in the container are obtained
     * by calling the relevant new to allocate an object dynamically.
     * If this isn't the case, this isn't an appropriate allocator
     * 
     * \param p Pointer to object to be deleted
     */
    template <class P> inline void delete_if_ptr (P* const& p) { delete(p); }

    /*! If P isn't a pointer type, ignore this call */
    template <class P> inline void delete_if_ptr (P const&) { }

    /*! Destroy (but don't deallocate) the collection member.
     * 
     * Since the collection is expected to contain pointers to
     * objects allocated with new(), delete_if_ptr() is called
     * to delete the object at which the collection element points.
     * This checks the type of the collection element and will
     * not attempt to call delete on a non-pointer type.
     * 
     * However, one would normally expect the referenced object
     * to be deleted, then the pointer within the collection
     * destroyed.
     * 
     * \param p Pointer to object to be destroyed
     */
    template <class U> void destroy(U* p) noexcept
    {
        delete_if_ptr(*p);
        p->~U();
    }
};

/*! All UniqueAllocator must compare as equal */
template <class T, class U> bool
operator==(UniqueAllocator<T> const&, UniqueAllocator<U> const&) noexcept
{
    return true;
};

/*! Standard != operator supplied by negation of operator== */
template <class T, class U> bool
operator!=(UniqueAllocator<T> const& x, UniqueAllocator<U> const& y) noexcept
{
    return !(x == y);
};

} // end namespace slidingbuffer

#endif
