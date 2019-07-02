#ifndef GUARD_heap_block_20190207153217_
#define GUARD_heap_block_20190207153217_
/*
@file		heap_block.hpp
@author		Webstar
@date		2019-07-02 15:32
@version	0.0.1
@note		Developed for Visual C++ 15.0
@brief		...
*/

namespace vs
{
	namespace heapblock_helper
	{
		template <bool shouldThrow>
		struct ThrowOnFail { static void checkPointer(void*) {} };

		template<>
		struct ThrowOnFail<true> { static void checkPointer(void* data) { if (data == nullptr) throw std::bad_alloc(); } };
	}

	/// @brief Very simple container class to hold a pointer to some data on the heap.
	/// @remark When you need to allocate some heap storage for something, always try to use
	/// this class instead of allocating the memory directly using malloc/free.
	/// A HeapBlock<char> object can be treated in pretty much exactly the same way
	/// as an char*, but as long as you allocate it on the stack or as a class member,
	/// it's almost impossible for it to leak memory.
	/// It also makes your code much more concise and readable than doing the same thing
	/// using direct allocations,

	/// E.g. instead of this:
	/// @code
	/// int* temp = (int*) malloc (1024 * sizeof (int));
	/// memcpy (temp, xyz, 1024 * sizeof (int));
	/// free (temp);
	/// temp = (int*) calloc (2048 * sizeof (int));
	/// temp[0] = 1234;
	/// memcpy (foobar, temp, 2048 * sizeof (int));
	/// free (temp);
	/// @endcode

	/// ..you could just write this:
	/// @code
	/// HeapBlock<int> temp (1024);
	/// memcpy (temp, xyz, 1024 * sizeof (int));
	/// temp.calloc (2048);
	/// temp[0] = 1234;
	/// memcpy (foobar, temp, 2048 * sizeof (int));
	/// @endcode

	/// The class is extremely lightweight, containing only a pointer to the
	/// data, and exposes malloc/realloc/calloc/free methods that do the same jobs
	/// as their less object-oriented counterparts. Despite adding safety, you probably
	/// won't sacrifice any performance by using this in place of normal pointers.

	/// The throwOnFailure template parameter can be set to true if you'd like the class
	/// to throw a std::bad_alloc exception when an allocation fails. If this is false,
	/// then a failed allocation will just leave the heapblock with a null pointer (assuming
	/// that the system's malloc() function doesn't throw).
	template <class ElementType, bool throwOnFailure = false>
	class HeapBlock
	{
	public:
		/// @brief Creates a HeapBlock which is initially just a null pointer.
		/// @remark After creation, you can resize the array using the malloc(), calloc(), or realloc() methods.
		HeapBlock() noexcept  : data(nullptr)
		{
		}

		/// @brief Creates a HeapBlock containing a number of elements.
		/// @remark The contents of the block are undefined, as it will have been created by a
		/// malloc call.
		/// If you want an array of zero values, you can use the calloc() method or the
		/// other constructor that takes an InitialisationState parameter.
		explicit HeapBlock(const size_t numElements)
			: data(static_cast<ElementType*> (std::malloc(numElements * sizeof(ElementType))))
		{
			throwOnAllocationFailure();
		}

		/// @brief Creates a HeapBlock containing a number of elements.
		/// @remark The initialiseToZero parameter determines whether the new memory should be cleared,
		/// or left uninitialised.
		HeapBlock(const size_t numElements, const bool initialiseToZero)
			: data(static_cast<ElementType*> (initialiseToZero
				? std::calloc(numElements, sizeof(ElementType))
				: std::malloc(numElements * sizeof(ElementType))))
		{
			throwOnAllocationFailure();
		}

		/// Destructor.
		~HeapBlock()
		{
			std::free(data);
		}

		/// Move constructor
		HeapBlock(HeapBlock&& other) noexcept
			: data(other.data)
		{
			other.data = nullptr;
		}

		/// Move assignment operator
		HeapBlock& operator= (HeapBlock&& other) noexcept
		{
			std::swap(data, other.data);
			return *this;
		}

		/// @brief Returns a raw pointer to the allocated data.
		/// @remark This may be a null pointer if the data hasn't yet been allocated, 
		/// or if it has been freed by calling the free() method.
		inline operator ElementType*() const noexcept { return data; }


		/// @brief Returns a raw pointer to the allocated data.
		/// @remark This may be a null pointer if the data hasn't yet been allocated, 
		/// or if it has been freed by calling the free() method.
		inline ElementType* get() const noexcept { return data; }

		/// @brief Returns a raw pointer to the allocated data.
		/// @remark This may be a null pointer if the data hasn't yet been allocated, 
		/// or if it has been freed by calling the free() method.
		inline ElementType* getData() const noexcept { return data; }

		/// @brief Returns a void pointer to the allocated data.
		/// @remark This may be a null pointer if the data hasn't yet been allocated, 
		/// or if it has been freed by calling the free() method.
		inline operator void*() const noexcept { return static_cast<void*> (data); }

		/// @brief Returns a void pointer to the allocated data.
		/// @remark This may be a null pointer if the data hasn't yet been allocated, 
		/// or if it has been freed by calling the free() method.
		inline operator const void*() const noexcept { return static_cast<const void*> (data); }

		/// @brief Lets you use indirect calls to the first element in the array.
		/// @remark Obviously this will cause problems if the array hasn't been initialised, 
		/// because it'll be referencing a null pointer.
		inline ElementType* operator->() const  noexcept { return data; }

		/// @brief  Returns a reference to one of the data elements.
		/// @remark Obviously there's no bounds-checking here, as this object is just a 
		/// dumb pointer and has no idea of the size it currently has allocated.
		template <typename IndexType>
		inline ElementType& operator[] (IndexType index) const noexcept { return data[index]; }

		/// @brief Returns a pointer to a data element at an offset from the start of the array.
		/// @remark This is the same as doing pointer arithmetic on the raw pointer itself.
		template <typename IndexType>
		inline ElementType* operator+ (IndexType index) const noexcept { return data + index; }

		/// @brief Compares the pointer with another pointer.
		/// @remark This can be handy for checking whether this is a null pointer.
		inline bool operator== (const ElementType* const otherPointer) const noexcept { return otherPointer == data; }

		/// @brief Compares the pointer with another pointer.
		/// @remark This can be handy for checking whether this is a null pointer.
		inline bool operator!= (const ElementType* const otherPointer) const noexcept { return otherPointer != data; }

		/// @brief Allocates a specified amount of memory.
		/// @remark This uses the normal malloc to allocate an amount of memory for this object.
		/// Any previously allocated memory will be freed by this method.
		/// The number of bytes allocated will be (newNumElements * elementSize). Normally
		/// you wouldn't need to specify the second parameter, but it can be handy if you need
		/// to allocate a size in bytes rather than in terms of the number of elements.
		/// The data that is allocated will be freed when this object is deleted, or when you
		/// call free() or any of the allocation methods.
		void malloc(const size_t newNumElements, const size_t elementSize = sizeof(ElementType))
		{
			std::free(data);
			data = static_cast<ElementType*> (std::malloc(newNumElements * elementSize));
			throwOnAllocationFailure();
		}

		/// @brief Allocates a specified amount of memory and clears it.
		/// @remark This does the same job as the malloc() method, but clears the memory that it allocates.
		void calloc(const size_t newNumElements, const size_t elementSize = sizeof(ElementType))
		{
			std::free(data);
			data = static_cast<ElementType*> (std::calloc(newNumElements, elementSize));
			throwOnAllocationFailure();
		}

		/// @brief Allocates a specified amount of memory and optionally clears it.
		/// @remark This does the same job as either malloc() or calloc(), depending on the initialiseToZero parameter.
		void allocate(const size_t newNumElements, bool initialiseToZero)
		{
			std::free(data);
			data = static_cast<ElementType*> (initialiseToZero
				? std::calloc(newNumElements, sizeof(ElementType))
				: std::malloc(newNumElements * sizeof(ElementType)));
			throwOnAllocationFailure();
		}

		/// @brief Re-allocates a specified amount of memory.
		/// @remark The semantics of this method are the same as malloc() and calloc(), 
		/// but it uses realloc() to keep as much of the existing data as possible.
		void realloc(const size_t newNumElements, const size_t elementSize = sizeof(ElementType))
		{
			data = static_cast<ElementType*> (data == nullptr ? std::malloc(newNumElements * elementSize)
				: std::realloc(data, newNumElements * elementSize));
			throwOnAllocationFailure();
		}

		/// @brief Frees any currently-allocated data.
		/// @remark This will free the data and reset this object to be a null pointer.
		void free() noexcept
		{
			std::free(data);
			data = nullptr;
		}

		/// @brief Swaps this object's data with the data of another HeapBlock.
		/// @remark The two objects simply exchange their data pointers.
		template <bool otherBlockThrows>
		void swapWith(HeapBlock<ElementType, otherBlockThrows>& other) noexcept
		{
			std::swap(data, other.data);
		}

		/// @brief This fills the block with zeros, up to the number of elements specified.
		/// @remark Since the block has no way of knowing its own size, you must make sure that the number of
		/// elements you specify doesn't exceed the allocated size.
		void clear(size_t numElements) noexcept
		{
			zeromem(data, sizeof(ElementType) * numElements);
		}

		/// This typedef can be used to get the type of the heapblock's elements.
		typedef ElementType Type;

	private:
		ElementType* data;

		void throwOnAllocationFailure() const
		{
			heapblock_helper::ThrowOnFail<throwOnFailure>::checkPointer(data);
		}
	};
}
/*
=============================================================
Copyright Venatio Studios 2019
=============================================================
Revision History

0.0.1 : 2019-07-02 15:32
#vNext
=============================================================
*/

#endif