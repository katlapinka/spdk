/*-
 *   BSD LICENSE
 *
 *   Copyright (c) Intel Corporation.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "spdk/stdinc.h"
#include "spdk_cunit.h"
#include "nvme/nvme_fabric.c"
#include "common/lib/test_env.c"

SPDK_LOG_REGISTER_COMPONENT(nvme)

struct spdk_nvmf_fabric_prop_set_cmd g_ut_cmd = {};
struct spdk_nvmf_fabric_prop_get_rsp g_ut_response = {};

DEFINE_STUB_V(nvme_completion_poll_cb, (void *arg, const struct spdk_nvme_cpl *cpl));

DEFINE_STUB(nvme_wait_for_completion, int,
	    (struct spdk_nvme_qpair *qpair,
	     struct nvme_completion_poll_status *status), 0);

DEFINE_STUB_V(spdk_nvme_ctrlr_get_default_ctrlr_opts,
	      (struct spdk_nvme_ctrlr_opts *opts, size_t opts_size));

DEFINE_STUB(nvme_transport_ctrlr_set_reg_4, int,
	    (struct spdk_nvme_ctrlr *ctrlr,
	     uint32_t offset, uint32_t value), 0);

DEFINE_STUB_V(nvme_ctrlr_destruct, (struct spdk_nvme_ctrlr *ctrlr));

DEFINE_STUB(nvme_ctrlr_cmd_identify, int,
	    (struct spdk_nvme_ctrlr *ctrlr, uint8_t cns, uint16_t cntid,
	     uint32_t nsid, uint8_t csi, void *payload, size_t payload_size,
	     spdk_nvme_cmd_cb cb_fn, void *cb_arg), 0);

DEFINE_STUB_V(nvme_ctrlr_connected, (struct spdk_nvme_probe_ctx *probe_ctx,
				     struct spdk_nvme_ctrlr *ctrlr));
DEFINE_STUB(nvme_ctrlr_add_process, int,
	    (struct spdk_nvme_ctrlr *ctrlr, void *devhandle), 0);

DEFINE_STUB(spdk_nvme_ctrlr_cmd_get_log_page, int,
	    (struct spdk_nvme_ctrlr *ctrlr, uint8_t log_page,
	     uint32_t nsid, void *payload, uint32_t payload_size,
	     uint64_t offset, spdk_nvme_cmd_cb cb_fn, void *cb_arg), 0);

DEFINE_STUB(spdk_nvme_transport_id_populate_trstring, int,
	    (struct spdk_nvme_transport_id *trid, const char *trstring), 0);

DEFINE_STUB(spdk_nvme_transport_available_by_name, bool,
	    (const char *transport_name), true);

DEFINE_STUB(nvme_transport_ctrlr_construct, struct spdk_nvme_ctrlr *,
	    (const struct spdk_nvme_transport_id *trid,
	     const struct spdk_nvme_ctrlr_opts *opts,
	     void *devhandle), NULL);

DEFINE_STUB(nvme_ctrlr_probe, int,
	    (const struct spdk_nvme_transport_id *trid,
	     struct spdk_nvme_probe_ctx *probe_ctx, void *devhandle), 0);

DEFINE_STUB(spdk_nvme_ctrlr_cmd_io_raw, int, (struct spdk_nvme_ctrlr *ctrlr,
		struct spdk_nvme_qpair *qpair, struct spdk_nvme_cmd *cmd, void *buf,
		uint32_t len, spdk_nvme_cmd_cb cb_fn, void *cb_arg), 0);

DEFINE_STUB(nvme_wait_for_completion_timeout, int,
	    (struct spdk_nvme_qpair *qpair,
	     struct nvme_completion_poll_status *status,
	     uint64_t timeout_in_usecs), 0);

DEFINE_STUB(spdk_nvme_transport_id_trtype_str, const char *,
	    (enum spdk_nvme_transport_type trtype), NULL);

DEFINE_STUB(spdk_nvme_transport_id_adrfam_str, const char *,
	    (enum spdk_nvmf_adrfam adrfam), NULL);

DEFINE_RETURN_MOCK(spdk_nvme_ctrlr_cmd_admin_raw, int);
int
spdk_nvme_ctrlr_cmd_admin_raw(struct spdk_nvme_ctrlr *ctrlr,
			      struct spdk_nvme_cmd *cmd,
			      void *buf, uint32_t len,
			      spdk_nvme_cmd_cb cb_fn, void *cb_arg)
{
	struct spdk_nvmf_fabric_prop_set_cmd *cmd_tmp = (void *)cmd;
	struct nvme_completion_poll_status *status = cb_arg;
	struct spdk_nvmf_fabric_prop_get_rsp *response = (void *)&status->cpl;

	g_ut_cmd.opcode = cmd_tmp->opcode;
	g_ut_cmd.fctype = cmd_tmp->fctype;
	g_ut_cmd.ofst = cmd_tmp->ofst;
	g_ut_cmd.attrib.size = cmd_tmp->attrib.size;

	if (cmd_tmp->fctype == SPDK_NVMF_FABRIC_COMMAND_PROPERTY_SET) {
		g_ut_cmd.value.u64 = cmd_tmp->value.u64;
	} else if (cmd_tmp->fctype == SPDK_NVMF_FABRIC_COMMAND_PROPERTY_GET) {
		memcpy(&g_ut_response, response, sizeof(g_ut_response));
	}

	HANDLE_RETURN_MOCK(spdk_nvme_ctrlr_cmd_admin_raw);
	return 0;
}

static void
test_nvme_fabric_prop_set_cmd(void)
{
	int rc;
	struct spdk_nvme_ctrlr ctrlr = {};

	memset(&g_ut_cmd, 0, sizeof(g_ut_cmd));

	rc = nvme_fabric_prop_set_cmd(&ctrlr, 1024, SPDK_NVMF_PROP_SIZE_8, 4096);
	CU_ASSERT(rc == 0);
	CU_ASSERT(g_ut_cmd.opcode == SPDK_NVME_OPC_FABRIC);
	CU_ASSERT(g_ut_cmd.fctype == SPDK_NVMF_FABRIC_COMMAND_PROPERTY_SET);
	CU_ASSERT(g_ut_cmd.ofst == 1024);
	CU_ASSERT(g_ut_cmd.attrib.size == SPDK_NVMF_PROP_SIZE_8);
	CU_ASSERT(g_ut_cmd.value.u64 == 4096);
}

static void
test_nvme_fabric_prop_get_cmd(void)
{
	int rc;
	uint64_t value;
	struct spdk_nvme_ctrlr ctrlr = {};

	memset(&g_ut_cmd, 0, sizeof(g_ut_cmd));
	memset(&g_ut_response, 0, sizeof(g_ut_response));
	value = 0xFFDEADBEEF;

	/* Case 1: size is SPDK_NVMF_PROP_SIZE_4 */
	rc = nvme_fabric_prop_get_cmd(&ctrlr, 1024, SPDK_NVMF_PROP_SIZE_4, &value);
	CU_ASSERT(rc == 0);
	CU_ASSERT(g_ut_cmd.opcode == SPDK_NVME_OPC_FABRIC);
	CU_ASSERT(g_ut_cmd.fctype == SPDK_NVMF_FABRIC_COMMAND_PROPERTY_GET);
	CU_ASSERT(g_ut_cmd.ofst == 1024);
	CU_ASSERT(g_ut_cmd.attrib.size == SPDK_NVMF_PROP_SIZE_4);
	CU_ASSERT(g_ut_response.value.u32.low == (value & 0xFFFFFFFF));

	/* Case 2: size is SPDK_NVMF_PROP_SIZE_8 */
	memset(&g_ut_cmd, 0, sizeof(g_ut_cmd));
	memset(&g_ut_response, 0, sizeof(g_ut_response));

	rc = nvme_fabric_prop_get_cmd(&ctrlr, 1024, SPDK_NVMF_PROP_SIZE_8, &value);
	CU_ASSERT(rc == 0);
	CU_ASSERT(g_ut_cmd.opcode == SPDK_NVME_OPC_FABRIC);
	CU_ASSERT(g_ut_cmd.fctype == SPDK_NVMF_FABRIC_COMMAND_PROPERTY_GET);
	CU_ASSERT(g_ut_cmd.ofst == 1024);
	CU_ASSERT(g_ut_cmd.attrib.size == SPDK_NVMF_PROP_SIZE_8);
	CU_ASSERT(g_ut_response.value.u64 == value);
}

int main(int argc, char **argv)
{
	CU_pSuite	suite = NULL;
	unsigned int	num_failures;

	CU_set_error_action(CUEA_ABORT);
	CU_initialize_registry();

	suite = CU_add_suite("nvme_fabric", NULL, NULL);
	CU_ADD_TEST(suite, test_nvme_fabric_prop_set_cmd);
	CU_ADD_TEST(suite, test_nvme_fabric_prop_get_cmd);

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	num_failures = CU_get_number_of_failures();
	CU_cleanup_registry();
	return num_failures;
}
