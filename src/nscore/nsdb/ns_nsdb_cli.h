/// Copyright 2026 Theo Lincke
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.

#ifndef NS_NSDB_CLI_H
#define NS_NSDB_CLI_H

#include "core/ns_alloc.h"
#include "core/ns_dbl_buffer.h"
#include "core/ns_error.h"
#include "nscore/txn_table/ns_txn_table.h"
#include "nscore/variables/ns_variables.h"

/******************************************************************************
 * SECTION: NSDB CLI
 * ----------------------------------------------------------------------------
 * @brief A stateful cli tool for nsdb
 *
 * Usage:
 *
 *
 * nscli_init(&cli, dbname);
 *
 * while(true) {
 *    nscli_step_init(&cli);
 *
 *    switch(nscli_step_read_stdin(&cli)) {
 *        case CMD_FATAL: {
 *            nsdb_perror(cli.db, "error: ");
 *            goto complete;
 *        }
 *
 *        case CMD_NOTHING_TO_DO: {
 *          break;
 *        }
 *
 *        case CMD_RUN: {
 *          switch(nscli_step_execute(&cli)) {
 *            case EXE_ERROR: {
 *                nsdb_perror(cli.db, "error: ");
 *                break;
 *            }
 *            case EXE_SUCCESS: {
 *                break;
 *            }
 *            case EXE_EXIT: {
 *                goto complete;
 *            }
 *          }
 *        }
 *    }
 * }
 *
 ******************************************************************************/

struct nscli
{
  struct nsdb      *db;         // The Database
  struct dbl_buffer stmt;       // Statement
  struct allocator  step_alloc; // Allocator for anything per step
};

err_t nscli_init (struct nscli *cli, const char *dbname);
err_t nscli_step_init (struct nscli *cli);

enum nscli_read_result
{
  CMD_NOTHING_TO_DO,
  CMD_FATAL,
  CMD_RUN,
} nscli_step_read_stdin (struct nscli *cli);

enum nscli_execute_result
{
  EXE_SUCCESS,
  EXE_ERROR,
  EXE_EXIT,
} nscli_step_execute (struct nscli *cli);

void nscli_step_clean (struct nscli *cli);
void nscli_close (struct nscli *cli);

#endif
