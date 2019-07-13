#include <iostream>
#include <list>
#include <typeinfo>
#include <cstdlib>
#include "gc_details.h"
#include "gc_iterator.h"
/*
    Pointer implements a pointer type that uses
    garbage collection to release unused memory.
    A Pointer must only be used to point to memory
    that was dynamically allocated using new.
    When used to refer to an allocated array,
    specify the array size.
*/
template <class T, int size = 0>
class Pointer{
private:
    // refContainer maintains the garbage collection list.
    static std::list<PtrDetails<T>> refContainer;
    // addr points to the allocated memory to which
    // this Pointer pointer currently points.
    T *addr;
    /*  isArray is true if this Pointer points
        to an allocated array. It is false
        otherwise. 
    */
    bool isArray; 
    // true if pointing to array
    // If this Pointer is pointing to an allocated
    // array, then arraySize contains its size.
    unsigned arraySize; // size of the array
    static bool first; // true when first Pointer is created
    // Return an iterator to pointer details in refContainer.
    typename std::list<PtrDetails<T>>::iterator findPtrInfo(T *ptr);
public:
    // Define an iterator type for Pointer<T>.
    typedef Iter<T> GCiterator;
    // Empty constructor
    // NOTE: templates aren't able to have prototypes with default arguments
    // this is why constructor is designed like this:
    Pointer(){
        Pointer(NULL);
    }
    Pointer(T*);
    // Copy constructor.
    Pointer(const Pointer &);
    // Destructor for Pointer.
    ~Pointer();
    // Collect garbage. Returns true if at least
    // one object was freed.
    static bool collect();
    // Overload assignment of pointer to Pointer.
    T *operator=(T *t);
    // Overload assignment of Pointer to Pointer.
    Pointer &operator=(Pointer &rv);
    // Return a reference to the object pointed
    // to by this Pointer.
    T &operator*(){
        return *addr;
    }
    // Return the address being pointed to.
    T *operator->() { return addr; }
    // Return a reference to the object at the
    // index specified by i.
    T &operator[](int i){ return addr[i];}
    // Conversion function to T *.
    operator T *() { return addr; }
    // Return an Iter to the start of the allocated memory.
    Iter<T> begin(){
        int _size;
        if (isArray)
            _size = arraySize;
        else
            _size = 1;
        return Iter<T>(addr, addr, addr + _size);
    }
    // Return an Iter to one past the end of an allocated array.
    Iter<T> end(){
        int _size;
        if (isArray)
            _size = arraySize;
        else
            _size = 1;
        return Iter<T>(addr + _size, addr, addr + _size);
    }
    // Return the size of refContainer for this type of Pointer.
    static int refContainerSize() { return refContainer.size(); }
    // A utility function that displays refContainer.
    static void showlist();
    // Clear refContainer when program exits.
    static void shutdown();
};

// STATIC INITIALIZATION
// Creates storage for the static variables
template <class T, int size>
std::list<PtrDetails<T>> Pointer<T, size>::refContainer;
template <class T, int size>
bool Pointer<T, size>::first = true;

// Constructor for both initialized and uninitialized objects. -> see class interface
template<class T,int size>
Pointer<T,size>::Pointer(T *t){

    if (first)
        atexit(shutdown);
    first = false;

    typename std::list<PtrDetails<T>>::iterator ptr;
    showlist();
    ptr = findPtrInfo(t);
    
    if (!ptr->refcount) {
        PtrDetails<T> newPtr(t);
        refContainer.push_back(newPtr);
    }
    else {
        ptr->refcount++;
    }
    
    showlist();
    
    addr = t;
    
    arraySize = 0;
    isArray = false;

}
// Copy constructor.
template< class T, int size>
Pointer<T,size>::Pointer(const Pointer &ob){

    typename std::list<PtrDetails<T>>::iterator ptr;
    ptr = findPtrInfo(ob.addr);
    
    addr = ob.addr;
    ptr->refcount++;
    arraySize = ob.arraySize;
    ptr->arraySize = ob.arraySize;
    
    if (ob.arraySize > 0) {
        isArray = true;
        ptr->isArray = isArray;
    }
    else {
        isArray = false;
        ptr->isArray = isArray;
    }
    
}

// Destructor for Pointer.
template <class T, int size>
Pointer<T, size>::~Pointer(){
    
    typename std::list<PtrDetails<T>>::iterator ptr;
    ptr = findPtrInfo(addr);
    if (ptr->refcount)
        ptr->refcount--; 
    collect();
    // For real use, you might want to collect unused memory less frequently,
    // such as after refContainer has reached a certain size, after a certain number of Pointers have gone out of scope,
    // or when memory is low.
}

// Collect garbage. Returns true if at least
// one object was freed.
template <class T, int size>
bool Pointer<T, size>::collect(){

    bool freed = false;
    typename std::list<PtrDetails<T> >::iterator ptr;
    do
    {
    
        for (ptr = refContainer.begin(); ptr != refContainer.end(); ptr++){
            if (ptr->refcount > 0)
            	continue;
            freed = true;
            
            if (ptr->memPtr) {
                if (ptr->isArray){
                    delete[] ptr->memPtr; // delete array
                }
                else{
                    delete ptr->memPtr; // delete single element
                }
            }
            
            refContainer.remove(*ptr);
            break;
        }
    } while (ptr != refContainer.end());
    
    return freed;
    // Note: collect() will be called in the destructor
}

// Overload assignment of pointer to Pointer.
template <class T, int size>
T *Pointer<T, size>::operator=(T *t){

    delete this;
    typename std::list<PtrDetails<T>>::iterator ptr;
    ptr = findPtrInfo(t); 
    showlist();
    
    if (!ptr->refcount) {
        PtrDetails<T> newPtr(t);
        refContainer.push_back(newPtr);
    }
    else {
        ptr->refcount++;
    }
    
    showlist();
    this->addr = t;
    this->isArray = false;
    
    return *this;

}
// Overload assignment of Pointer to Pointer.
template <class T, int size>
Pointer<T, size> &Pointer<T, size>::operator=(Pointer &rv){
    Pointer ptr;
    typename std::list<PtrDetails<T>>::iterator p;
    p = findPtrInfo(addr); 
    p->refcount--;
    rv->refcount++;
    ptr.addr = rv.addr;
    return ptr;
}

// A utility function that displays refContainer.
template <class T, int size>
void Pointer<T, size>::showlist(){
    typename std::list<PtrDetails<T> >::iterator ptr;
    std::cout << "refContainer<" << typeid(T).name() << ", " << size << ">:\n";
    std::cout << "memPtr refcount value\n ";
    if (refContainer.begin() == refContainer.end())
    {
        std::cout << " Container is empty!\n\n ";
    }
    for (ptr = refContainer.begin(); ptr != refContainer.end(); ptr++)
    {
        std::cout << "[" << (void *)ptr->memPtr << "]"
             << " " << ptr->refcount << " ";
        if (ptr->memPtr)
            std::cout << " " << *ptr->memPtr;
        else
            std::cout << "---";
        std::cout << std::endl;
    }
    std::cout << std::endl;
}
// Find a pointer in refContainer.
template <class T, int size>
typename std::list<PtrDetails<T>>::iterator
Pointer<T, size>::findPtrInfo(T *ptr){
    typename std::list<PtrDetails<T>>::iterator p;
    
    for (p = refContainer.begin(); p != refContainer.end(); p++)
        if (p->memPtr == ptr)
            return p;
            
    return p;
}
// Clear refContainer when program exits.
template <class T, int size>
void Pointer<T, size>::shutdown(){
    if (refContainerSize() == 0)
        return ; 
        
    typename std::list<PtrDetails<T>>::iterator ptr;
    
    for (ptr = refContainer.begin(); ptr != refContainer.end(); ptr++)
    {
        ptr->refcount = 0;
    }
    
    collect();
}
