/* Created by Taco on 3/30/25.
 * Purpose: Convert a binary file to a C++ header file containing a byte array.
 * Usage: Bin2Header -o <output_file> -n <array_name> <binary_file>
 * This program is automatically ran by the build system
 */

#include <algorithm>
#include <cctype>
#include <expected>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

class Bin2Header {
  public:
  explicit Bin2Header(const char* args[6])
      : input_file_name(args[5])
      , output_file_name(args[2])
      , header_name(args[4])
      , in_file(input_file_name, std::ios::binary)
      , out_file(output_file_name) {
    if (!in_file.is_open()) {
      std::cerr << "Error opening input file: " << input_file_name << std::endl;
      exit(1);
    }
    if (!out_file.is_open()) {
      std::cerr << "Error opening output file: " << output_file_name
                << std::endl;
      exit(1);
    }
  }

  std::expected<void, std::string> WriteHeader() {
    std::string header_guard = header_name;
    ToUpperCase(header_guard);

    out_file << "#ifndef " << header_guard << "_H\n";
    out_file << "#define " << header_guard << "_H\n\n";
    out_file << "inline const unsigned char " << header_name << "[] = {\n\t";


    auto bin = ReadBin();
    if (!bin) {
      return std::unexpected(bin.error());
    }
    const auto bin_data = bin.value();

    for (size_t i = 0; i < bin_data.size(); ++i) {
      out_file << "0x" << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<int>(bin_data[i]);
      if (i != bin_data.size() - 1) {
        out_file << ", ";
        if ((i + 1) % 12 == 0) out_file << "\n\t";
      }
    }

    out_file << "\n};\n\n";
    out_file << "#endif /* " << header_guard << "_H */\n";
    out_file.close();
    return {};
  }

  private:
  std::string input_file_name;
  std::string output_file_name;
  std::string header_name;
  std::ifstream in_file;
  std::ofstream out_file;


  std::expected<std::vector<uint8_t>, std::string> ReadBin() {
    std::vector<uint8_t> bin((std::istreambuf_iterator(in_file)),
                             std::istreambuf_iterator<char>());
    in_file.close();

    if (bin.empty()) {
      return std::unexpected("Error reading binary file: " + input_file_name);
    }

    if (bin.size() != 32 && bin.size() != 64) {
      return std::unexpected(
          "Error: Binary file size is not 32 or 64 bytes: current size is " +
          std::to_string(bin.size()));
    }

    return bin;
  }

  static void ToUpperCase(std::string& str) {
#ifdef WIN32
    std::ranges::transform(str, str.begin(), toupper);
#else
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
#endif
  }
};


int main(const int argc, const char** argv) {
  if (argc != 6 || std::string(argv[1]) != "-o" ||
      std::string(argv[3]) != "-n") {
    std::cerr << "Usage: " << argv[0]
              << " -o <output_file> -n <array_name> <binary_file>\n";
    return 1;
  }

  auto bin2header = Bin2Header(argv);
  if (auto result = bin2header.WriteHeader(); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  std::cout << "Header file generated successfully: " << argv[2] << std::endl;

  return 0;
}
