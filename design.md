# Concepts

## Reading

### Readable

```cpp
struct readable {
    expected<streamsize, error> read(byte_span data);
    bool putback(byte b);
};
```

### VectorReadable

```cpp
struct vector_readable {
    expected<span<byte_span>, error> vread(std::initializer_list<byte_span> list, streamoff off = 0);
};
```

### DirectReadable

```cpp
struct direct_readable {
    const_byte_span input_range();
};
```

### AsyncVectorReadable

TODO

## Writing

### Writable

```cpp
struct writable {
    expected<streamsize, error> write(const_byte_span data);
    error sync();
};
```

### VectorWritable

```cpp
struct vector_writable {
    expected<streamsize, error> vwrite(std::initializer_list<const_byte_span> list, streamoff off = 0);
};
```

### DirectWritable

```cpp
struct direct_writable {
    byte_span output_range();
};
```

### AsyncVectorWritable

TODO

## Seeking

### InputSeekable

```cpp
struct input_seekable {
    expected<streampos, error> seek_in(streampos pos);
    expected<streampos, error> seek_in(streamoff off, seekdir dir);
};
```

### OutputSeekable

```cpp
struct output_seekable {
    expected<streampos, error> seek_out(streampos pos);
    expected<streampos, error> seek_out(streamoff off, seekdir dir);
};
```

### Seekable

```cpp
struct seekable : input_seekable, output_seekable {
    expected<streampos, error> seek(streampos pos);
    expected<streampos, error> seek(streamoff off, seekdir dir);
};
```

# Types

## Streams

### `stream_base`

```cpp
class stream_base {
    
};
```
