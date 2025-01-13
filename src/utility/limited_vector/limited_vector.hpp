#ifndef LIMITED_VECTOR_HPP
#define LIMITED_VECTOR_HPP

#include <vector>

/**
 * @brief A vector-like container that maintains a maximum size.
 *        Automatically removes the oldest elements when the size exceeds the limit.
 *
 * @tparam T The type of elements stored in the container.
 */
template <typename T> class LimitedVector {
  public:
    /**
     * @brief Constructor to initialize the vector with a maximum size.
     *
     * @param max_size The maximum number of elements the vector can hold.
     */
    explicit LimitedVector(size_t max_size);

    /**
     * @brief Adds an element to the vector. If the vector exceeds the maximum size,
     *        the oldest element is removed.
     *
     * @param value The element to be added.
     */
    void push_back(const T &value);

    /**
     * @brief Access an element by index.
     *
     * @param index The index of the element to access.
     * @return The element at the specified index.
     */
    T &operator[](size_t index);

    /**
     * @brief Access an element by index (const version).
     *
     * @param index The index of the element to access.
     * @return The element at the specified index.
     */
    const T &operator[](size_t index) const;

    /**
     * @brief Gets the current size of the vector.
     *
     * @return The number of elements in the vector.
     */
    size_t size() const;

    /**
     * @brief Checks if the vector is empty.
     *
     * @return True if the vector is empty, false otherwise.
     */
    bool empty() const;

    /**
     * @brief Clears all elements in the vector.
     */
    void clear();

  private:
    size_t max_size_;     ///< The maximum number of elements the vector can hold.
    std::vector<T> data_; ///< The underlying storage for the vector elements.
};

#include "limited_vector.tpp"

#endif // LIMITED_VECTOR_HPP
