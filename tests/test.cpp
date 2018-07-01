#include <spio/spio.h>

int main()
{
    {
        spio::stdio_handle_device out(stdout);
        //out.write(spio::gsl::byte_span("Say something\n"));
        spio::write_all(out, spio::gsl::byte_span("Say something\n"));

        spio::stdio_handle_device in(stdin);
        std::string buf;
        gsl::byte ch{};
        bool eof = false;
        while (!eof) {
            auto ret = in.bread(ch);
            if (ret) {
                out.write(spio::gsl::byte_span("Error: "));
                out.write(spio::gsl::as_bytes(spio::gsl::make_span(
                    ret.error().what(), std::strlen(ret.error().what()))));
            }
            if (ch != gsl::to_byte('\n')) {
                buf.push_back(gsl::to_uchar(ch));
            }
            else {
                break;
            }
        }
        in.putback(ch);

        buf = "You said: '" + buf;
        buf.push_back('\'');

        out.write(
            spio::gsl::as_bytes(spio::gsl::make_span(&buf[0], buf.size())));
    }

    {
        std::array<char, 32> arr;
        arr.fill(0);

        spio::memory_sink write(spio::gsl::as_writeable_bytes(
            spio::gsl::make_span(arr.data(), 32)));
        std::fill(write.output().begin(), write.output().end(),
                  spio::gsl::to_byte('a'));

        spio::memory_source read(
            spio::gsl::as_bytes(spio::gsl::make_span(arr.data(), 32)));
        spio::stdio_handle_device out(stdout);
        out.write(read.input());
    }

#if SPIO_USE_AFIO
    {
        auto file_read = spio::afio::file({}, "file.txt").value();
        auto read = spio::afio_file_device(file_read);

        auto file_write =
            spio::afio::file({}, "file.txt",
                             spio::afio::file_handle::mode::write)
                .value();
        auto write = spio::afio_file_device(file_write);

        std::vector<spio::gsl::byte> buf(file_read.maximum_extent().value());

        auto read_buf =
            spio::gsl::as_writeable_bytes(spio::gsl::make_span(buf));
        auto ret = *read.vread(spio::gsl::make_span(&read_buf, 1), 0);
        buf.resize(ret[0].size());

        file_write.truncate(0).value();
        auto write_buf = spio::gsl::as_bytes(ret[0]);
        *write.vwrite(spio::gsl::make_span(&write_buf, 1), 0);
    }
#endif
}
