#include "UUID.h"

String UUIDGenerator::generateUUIDv4() {
      String uuid = "";
      const char *hexChars = "0123456789abcdef";

      // Tạo 8 ký tự đầu tiên
      for (int i = 0; i < 8; i++) {
            uuid += hexChars[random(16)];
      }
      uuid += "-";

      // Tạo 4 ký tự tiếp theo
      for (int i = 0; i < 4; i++) {
            uuid += hexChars[random(16)];
      }
      uuid += "-";

      // Tạo 4 ký tự tiếp theo (UUID v4 xác định ký tự đầu tiên là '4')
      uuid += "4";
      for (int i = 0; i < 3; i++) {
            uuid += hexChars[random(16)];
      }
      uuid += "-";

      // Tạo 4 ký tự tiếp theo (phần variant, ký tự đầu tiên là '8', '9', 'a' hoặc 'b')
      uuid += hexChars[8 + random(4)];
      for (int i = 0; i < 3; i++) {
            uuid += hexChars[random(16)];
      }
      uuid += "-";

      // Tạo 12 ký tự cuối cùng
      for (int i = 0; i < 12; i++) {
            uuid += hexChars[random(16)];
      }

      return uuid;
}