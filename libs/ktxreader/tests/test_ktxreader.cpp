/*
 * Copyright 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ktxreader/Ktx1Reader.h>
#include <ktxreader/Ktx2Reader.h>

#include <filament/Engine.h>
#include <filament/Texture.h>

#include <gtest/gtest.h>
#include <utils/Path.h>

#include <exception>
#include <fstream>
#include <vector>

using namespace filament;
using namespace std;

using utils::Path;

class KtxReaderTest : public testing::Test {
protected:
    void SetUp() override {
        engine = Engine::create(Engine::Backend::NOOP);
    }

    void TearDown() override {
        Engine::destroy(&engine);
    }

    Engine* engine = nullptr;
};

static vector<uint8_t> readFile(const Path& inputPath) {
    std::ifstream file(inputPath, ios::binary);
    return vector<uint8_t>((istreambuf_iterator<char>(file)), {});
}

TEST_F(KtxReaderTest, Ktx1) {
    const utils::Path parent = Path::getCurrentExecutable().getParent();
    const auto contents = readFile(parent + "lightroom_ibl.ktx");
    ASSERT_EQ(contents.size(), 129368);

    image::Ktx1Bundle bundle(contents.data(), contents.size());

    Texture* tex = ktxreader::Ktx1Reader::createTexture(engine, bundle, false,
            [](void* userdata) {  /* */ }, nullptr);

    ASSERT_NE(tex, nullptr);
    ASSERT_EQ(tex->getWidth(), 64);
    ASSERT_EQ(tex->getHeight(), 64);

    engine->destroy(tex);
}

TEST_F(KtxReaderTest, Ktx2) {
    const utils::Path parent = Path::getCurrentExecutable().getParent();
    const auto contents = readFile(parent + "color_grid_uastc_zstd.ktx2");
    ASSERT_EQ(contents.size(), 170512);

    ktxreader::Ktx2Reader reader(*engine);

    reader.requestFormat(Texture::InternalFormat::DXT5_SRGBA);
    reader.requestFormat(Texture::InternalFormat::DXT5_RGBA);

    // Uncompressed formats are lower priority, so they get added last.
    reader.requestFormat(Texture::InternalFormat::SRGB8_A8);
    reader.requestFormat(Texture::InternalFormat::RGBA8);

    Texture* tex = reader.load(contents.data(), contents.size(),
            ktxreader::Ktx2Reader::TransferFunction::sRGB);

    ASSERT_NE(tex, nullptr);
    ASSERT_EQ(tex->getFormat(), Texture::InternalFormat::DXT5_SRGBA);
    ASSERT_EQ(tex->getWidth(), 1024);

    engine->destroy(tex);
}


TEST_F(KtxReaderTest, Ktx2InitWrapRegression) {
    // Regression test for the KTX2 init() uint64 offset/length wrap family
// (11 consumers, one root): ktx2_transcoder::init() used to add the SGD
// / level byteOffset + byteLength in uint64, so a wrapping pair passed
// the bounds check and the transcoder then read from a wild pointer
// (SEGV / heap-buffer-overflow). init() now rejects any wrapping pair,
// so every trigger below must be rejected: load() returns nullptr (or
// throws) and the process must not crash. 18 of these 23 crashed through
// Ktx2Reader::load() before the fix; the other 5 (sgd_off_far,
// slice_desc, slice_s4_len12, t1_std_scheme4_wrap, t3_orig_scheme1_wrap)
// are basisu-level-only / redzone-dependent shapes that load() already
// rejected, and are covered at the transcoder level by the standalone
// ASan harness runs recorded with the fix. The seed file still
// transcodes (see the Ktx2 test above).
static const struct {
    const char* filename;
size_t size;
const char* consumer;
} kWrapTriggers[] = {
    { "m3_level_offset_wrap4g.ktx2", 170512, "C1 level offset wrap" },
    { "s1_sgd_wrap_1tb.ktx2", 170512, "C2 SGD offset wrap" },
    { "t1_std_scheme4_wrap.ktx2", 170512, "C3 SGD wrap, std descs" },
    { "t2_std_scheme5_wrap.ktx2", 170512, "C3 SGD wrap, std descs" },
    { "t3_orig_scheme1_wrap.ktx2", 170512, "C3 SGD wrap, orig descs" },
    { "sgd_len_wrap_ep.ktx2", 524, "C4 SGD length wrap, ep inflation" },
    { "sgd_len_wrap_sel.ktx2", 524, "C4 SGD length wrap, sel inflation" },
    { "slice_desc.ktx2", 524, "C5 SGD wrap, non-std slice descs" },
    { "slice_s4_len12.ktx2", 524, "C6 SGD wrap, std slice descs" },
    { "t1_tablescraft.ktx2", 524, "C7 SGD len wrap, 1st Huffman table" },
    { "t2_tablescraft.ktx2", 524, "C7 SGD len wrap, 1st Huffman table" },
    { "n1_2nd_table.ktx2", 524, "C8 SGD len wrap, 2nd Huffman table" },
    { "n4_2nd_table_t2.ktx2", 524, "C8 SGD len wrap, 2nd Huffman table" },
    { "n2_3rd_table.ktx2", 524, "C9 SGD len wrap, 3rd Huffman table" },
    { "n3_4th_table.ktx2", 524, "C10 SGD len wrap, 4th Huffman table" },
    { "n5_4th_table_m8.ktx2", 524, "C10 SGD len wrap, 4th Huffman table" },
    { "uastc_lvl_off_far.ktx2", 170512, "C1 extent, far level offset" },
    { "sgd_off_far.ktx2", 524, "C2 extent, far SGD offset" },
    { "A13_off2_64m1.ktx2", 524, "C11 level offset wrap, level0" },
    { "A14_off2_64m44.ktx2", 524, "C11 level offset wrap, level0" },
    { "F01_l1off2_64m1.ktx2", 612, "C11 level offset wrap, level1" },
    { "F02_l1off2_64m44.ktx2", 612, "C11 level offset wrap, level1" },
    { "F13_l0off2_64m1_v2.ktx2", 612, "C11 level offset wrap, level0 2-level" },
};
const utils::Path parent = Path::getCurrentExecutable().getParent();
for (const auto& trigger: kWrapTriggers) {
const auto contents = readFile(parent + trigger.filename);
ASSERT_EQ(contents.size(), trigger.size) << trigger.filename;
ktxreader::Ktx2Reader reader(*engine);
reader.requestFormat(Texture::InternalFormat::DXT5_SRGBA);
reader.requestFormat(Texture::InternalFormat::DXT5_RGBA);
reader.requestFormat(Texture::InternalFormat::SRGB8_A8);
reader.requestFormat(Texture::InternalFormat::RGBA8);
Texture* tex = nullptr;
try {
            tex = reader.load(contents.data(), contents.size(),
                    ktxreader::Ktx2Reader::TransferFunction::sRGB);
} catch (const std::exception&) {
// Rejected via exception: also acceptable, no crash.
    continue;
}
        ASSERT_EQ(tex, nullptr) << trigger.filename << " (" << trigger.consumer << ")";
        // Reaching here with tex == nullptr means init() rejected the
    // wrapping offset/length pair before any wild-pointer read.
}
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
