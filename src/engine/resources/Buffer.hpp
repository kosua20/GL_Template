#pragma once
#include "graphics/GPUObjects.hpp"
#include "Common.hpp"

/** \brief General purpose GPU/CPU buffer, without a CPU backing store.
 \details This allows the class to be template-free (to avoid exposing GLUtilities/GPUObjects details
 in headers), while preserving the "pass general Object to GLUtilities for setup/upload &
 GPUObject only stores ID and enums" approach followed by Texture and Mesh.
 In practice you will want to use Buffer to benefit from CPU storage and simplified upload.
 \ingroup Resources
 */
class BufferBase {
public:

	/** Constructor.
	\param sizeInBytes the size of the buffer in bytes
	\param atype the target of the buffer (uniform, index, vertex)
	\param ausage the update frequency of the buffer content
	 */
	BufferBase(size_t sizeInBytes, BufferType atype, DataUse ausage);

	/** Setup the buffer, allocating it on the GPU.
	 */
	void setup();

	/** Upload data to the buffer. You have to take care of synchronization if
	 updating a subregion of the buffer that is currently in use, except if
	 sizeInBytes == size of the buffer, in which case the current buffer is
	 orphaned and a new one used (if the driver is nice).
	 \param sizeInBytes the size of the data to upload, in bytes
	 \param data the data to upload
	 \param offset offset in the buffer
	 */
	void upload(size_t sizeInBytes, unsigned char * data, size_t offset);

	/** Download data from the buffer.
	 \param sizeInBytes the size of the data to download, in bytes
	 \param data the storage to download to
	 \param offset offset in the buffer
	 */
	void download(size_t sizeInBytes, unsigned char * data, size_t offset);

	/** Cleanup all data.
	 */
	void clean();

	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	BufferBase & operator=(const BufferBase &) = delete;

	/** Copy constructor (disabled). */
	BufferBase(const BufferBase &) = delete;

	/** Move assignment operator .
	 \return a reference to the object assigned to
	 */
	BufferBase & operator=(BufferBase &&) = delete;

	/** Move constructor. */
	BufferBase(BufferBase &&) = delete;

	/** Destructor. */
	~BufferBase();

	const size_t sizeMax; ///< Buffer size in bytes.
	const BufferType type; ///< Buffer type.
	const DataUse usage; ///< Buffer update frequency.

	std::unique_ptr<GPUBuffer> gpu; ///< The GPU data (optional).

};

/**
 \brief Represents a buffer containing arbitrary data, stored on the CPU and/or GPU.
 \ingroup Resources
 */
template<typename T>
class Buffer : public BufferBase {
public:

	/** Constructor.
	 \param count the number of elements
	 \param type the target of the buffer (uniform, index, vertex)
	 \param usage the update frequency of the buffer content
	 */
	Buffer(size_t count, BufferType type, DataUse usage);

	/** Accessor.
	 \param i the location of the item to retrieve
	 \return a reference to the item
	 */
	T & operator[](size_t i){
		return data[i];
	}

	/** Accessor.
	 \param i the location of the item to retrieve
	 \return a reference to the item
	 */
	const T & operator[](size_t i) const {
		return data[i];
	}

	/** Accessor.
	 \param i the location of the item to retrieve
	 \return a reference to the item
	 */
	T & at(size_t i){
		return data[i];
	}

	/** Accessor.
	 \param i the location of the item to retrieve
	 \return a reference to the item
	 */
	const T & at(size_t i) const {
		return data[i];
	}

	/** \return the CPU size of the buffer. */
	size_t size() const {
		return data.size();
	}

	/** Send the buffer data to the GPU.
	 Previously uploaded content will potentially be erased.
	 */
	void upload();

	/** Send part of the buffer data to the GPU.
	 \param count number of elements to upload
	 \param offset location of the first element to upload
	*/
	void upload(size_t offset, size_t count);

	/** Download data from the GPU buffer to the CPU.
	 If the CPU buffer was cleared, it will be reallocated.
	 */
	void download();

	/** Clear CPU data.
	 */
	void clearCPU();

	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	Buffer & operator=(const Buffer &) = delete;
	
	/** Copy constructor (disabled). */
	Buffer(const Buffer &) = delete;
	
	/** Move assignment operator .
	 \return a reference to the object assigned to
	 */
	Buffer & operator=(Buffer &&) = default;
	
	/** Move constructor. */
	Buffer(Buffer &&) = default;

	/** Destructor. */
	~Buffer();

	std::vector<T> data; ///< The CPU data.
	
};

template <typename T>
Buffer<T>::Buffer(size_t count, BufferType type, DataUse usage) :
	BufferBase(count * sizeof(T), type, usage) {
	data.resize(count);
}

template <typename T>
void Buffer<T>::upload() {
	BufferBase::upload(data.size() * sizeof(T), reinterpret_cast<unsigned char*>(data.data()), 0);
}

template <typename T>
void Buffer<T>::upload(size_t offset, size_t count) {
	BufferBase::upload(count * sizeof(T), reinterpret_cast<unsigned char*>(data.data()), offset);
}

template <typename T>
void Buffer<T>::download() {
	// Resize to make sure that we have enough room.
	data.resize(sizeMax/sizeof(T));
	BufferBase::download(data.size() * sizeof(T), reinterpret_cast<unsigned char*>(data.data()), 0);
}

template <typename T>
void Buffer<T>::clearCPU() {
	data.clear();
}

template <typename T>
Buffer<T>::~Buffer() {
	clearCPU();
	// The parent destructor will do the GPU cleaning.
}
