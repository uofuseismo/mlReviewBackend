#ifndef COMPRESS_HPP
#define COMPRESS_HPP
#include <string>
#ifndef WITH_ZLIB
#include <sstream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#endif
namespace
{

std::string compress(const std::string &data) 
{
#ifndef WITH_ZLIB
    boost::iostreams::filtering_streambuf<boost::iostreams::output>
        outputStream;
    outputStream.push(boost::iostreams::gzip_compressor());
    std::stringstream stringStream;
    outputStream.push(stringStream);
    boost::iostreams::copy(
        boost::iostreams::basic_array_source<char> (data.c_str(),
                                                    data.size()), outputStream);
    return stringStream.str();
#else
    return data;
#endif
}

std::string decompress(const std::string &cipherText)
{
#ifndef WITH_ZLIB
    std::stringstream stringStream;
    stringStream << cipherText;
    boost::iostreams::filtering_streambuf<boost::iostreams::input> inputStream;
    inputStream.push(boost::iostreams::gzip_decompressor());

    inputStream.push(stringStream);
    std::stringstream unpackedText;
    boost::iostreams::copy(inputStream, unpackedText);
    return unpackedText.str();
#else
    return cipherText;
#endif
}

}
#endif
