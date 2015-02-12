#ifndef _BASEALLOC_H
#define _BASEALLOC_H

#include <assert.h>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <cstddef>

#include <boost/limits.hpp>

class BaseAllocator
{
public:
  virtual void * allocate(std::size_t size) = 0;
  virtual void deallocateAll() = 0;
  virtual void deallocate(void * ptr) { }
  virtual ~BaseAllocator() {}
};

template<unsigned long blockSizeT = 4096>
class BlockAllocator : public BaseAllocator
{
public:
  static const std::size_t initialBlockMemory = 10;

  BlockAllocator()
    : m_blocks(), m_currentBlockFree(0),
      m_currentPtr(nullptr)
  { m_blocks.resize(initialBlockMemory); } 

  ~BlockAllocator()
  { deleteBlocks(); }

  void * allocate(std::size_t size) 
  {
    assert(size >  0);
    if (m_currentBlockFree >= size)
    {
      m_lastAllocated = m_currentPtr;
      m_currentPtr += size;
      m_currentBlockFree -= size;
      return m_lastAllocated;
    }
    else
    {
      size_t nextBlockSize = std::max(blockSizeT, ( 1 + size / blockSizeT ) * blockSizeT);
      m_blocks.push_back((char *) ::operator new(nextBlockSize));
      m_currentBlockFree = nextBlockSize - size;
      m_currentPtr = m_blocks.back() + size;  
      m_lastAllocated = m_blocks.back();
      return m_lastAllocated; 
    }
  }

  void deallocateAll() 
  { deleteBlocks(); }  

  void deallocate(void * ptr) 
  {
    if (ptr == m_lastAllocated)
    {
      m_currentBlockFree += ( m_currentPtr - m_lastAllocated);
      m_currentPtr = m_lastAllocated;
    }
  }

private:
  void deleteBlocks() 
  {
    for (std::vector<char *>::const_iterator it = m_blocks.begin();
      it != m_blocks.end(); ++it)
     ::operator delete(*it);
    m_blocks.clear();
    m_currentBlockFree = 0;          
    m_currentPtr = nullptr;
  }

  std::vector<char *> m_blocks; 
  std::size_t         m_currentBlockFree;          
  char *              m_currentPtr;
  char *              m_lastAllocated;
};

template<typename T>
class StlAllocator 
{
public : 
  typedef T value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;

public : 
  template<typename U>
  struct rebind 
  { typedef StlAllocator<U> other; };

public : 
  explicit StlAllocator(BaseAllocator * baseAllocator = nullptr)
    : m_baseAllocator(baseAllocator) 
  {}
  StlAllocator(BaseAllocator& baseAllocator)
    : m_baseAllocator(&baseAllocator) 
  {}
  StlAllocator(const StlAllocator& allocator) 
    : m_baseAllocator(&allocator.baseAllocator())
  {}
  template<typename U>
  StlAllocator(const StlAllocator<U>& allocator) 
    : m_baseAllocator(&allocator.baseAllocator())
  {}
  template<class U>
  StlAllocator& operator=(const StlAllocator<U>& other)
  {
    m_baseAllocator = &other.baseAllocator();
    return this;
  }

  pointer address(reference r) { return &r; }
  const_pointer address(const_reference r) { return &r; }

  pointer allocate(size_type cnt, 
    typename std::allocator<void>::const_pointer = 0) 
  { 
    assert(m_baseAllocator != nullptr);
    return reinterpret_cast<pointer>(m_baseAllocator->allocate(cnt * sizeof (T))); 
  }
  void deallocate(pointer p, size_t) 
  { 
    assert(m_baseAllocator != nullptr);
    if(p)
      m_baseAllocator->deallocate(p); 
  }

  size_type max_size() const 
  { return std::numeric_limits<size_type>::max() / sizeof(T); }

  void construct(pointer p, const T& t) { new(p) T(t); }
  void destroy(pointer p) { p->~T(); }

  bool operator==(StlAllocator const& a) const 
  { return m_baseAllocator == a.m_baseAllocator; }
  bool operator!=(StlAllocator const& a) const 
  { return !operator==(a); }

  BaseAllocator& baseAllocator() const
  { 
    assert(m_baseAllocator != nullptr);
    return *m_baseAllocator; 
  }

private:
  BaseAllocator* m_baseAllocator;
};  

inline void * operator new(std::size_t size, BaseAllocator& allocator)
{ return allocator.allocate(size); };

#endif
