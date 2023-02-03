#include <poll.h>

#include <database.hpp>
#include <term.hpp>

#include "ms.h"

using namespace System;

static ms_item_t sn_tools;

std::string Database::path_(std::string(getenv("HOME")) + "/.rm_database/");

Database::Key<uint8_t[32]> *sn;  // NOLINT(modernize-avoid-c-arrays)

Database::Database() {
  auto sn_cmd_fn = [](ms_item_t *item, int argc, char **argv) {
    MS_UNUSED(item);

    if (argc == 1) {
      ms_printf("-show        show SN code.");
      ms_enter();
      ms_printf("-set [code]  set  SN code.");
      ms_enter();
    } else if (argc == 2) {
      if (strcmp("show", argv[1]) == 0) {
        sn->Get();
        ms_printf("SN:%.32s", sn->data_);
        ms_enter();
      } else {
        ms_printf("Error command.");
        ms_enter();
      }
    } else if (argc == 3) {
      if (strcmp("set", argv[1]) == 0 && strlen(argv[2]) == 32) {
        bool check_ok = true;

        for (uint8_t i = 0; i < 32; i++) {
          if (isalnum(argv[2][i])) {
            sn->data_[i] = argv[2][i];
          } else {
            check_ok = false;
            sn->Get();
            break;
          }
        }

        if (check_ok) {
          sn->Set();
          ms_printf("SN:%.32s", sn->data_);
          ms_enter();
        } else {
          ms_printf("Error sn code format: %s", argv[2]);
          ms_enter();
        }
      } else {
        ms_printf("Error sn code format: %s", argv[2]);
        ms_enter();
      }
    }

    return 0;
  };

  poll(NULL, 0, 1);

  mkdir(path_.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);

  sn =
      new Database::Key<uint8_t[32]>("SN");  // NOLINT(modernize-avoid-c-arrays)
  ms_file_init(&sn_tools, "sn_tools", sn_cmd_fn, NULL, NULL);
  ms_cmd_add(&sn_tools);
}
