//
// DRNSF - An unofficial Crash Bandicoot level editor
// Copyright (C) 2017-2018  DRNSF contributors
//
// See the AUTHORS.md file for more details.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "common.hh"
#include "nsf.hh"
#include "misc.hh"

namespace drnsf {
namespace nsf {

// declared in res.hh
void spage::import_file(TRANSACT, const util::blob &data)
{
    assert_alive();

    util::binreader r;
    r.begin(data);

    // Ensure the page data is the correct size (64K).
    if (data.size() != page_size)
        throw res::import_error("nsf::spage: not 64K");

    // Read the page header.
    auto magic         = r.read_u16();
    auto type          = r.read_u16();
    auto cid           = r.read_u32();
    auto pagelet_count = r.read_u32();
    auto checksum      = r.read_u32();

    // Ensure the magic number is correct.
    if (magic != 0x1234)
        throw res::import_error("nsf::spage: bad magic number");

    // Read the pagelet offsets. Pagelets should be entries, but we treat
    // them as blobs instead so they can be totally unprocessed, etc.
    std::vector<uint32_t> pagelet_offsets(pagelet_count + 1);
    for (auto &&pagelet_offset : pagelet_offsets) {
        pagelet_offset = r.read_u32();
    }
    r.end_early();

    // Copy the data for each pagelet as a new raw_data asset. The caller
    // can later process these into entries if desired.
    std::vector<misc::raw_data::ref> pagelets(pagelet_count);
    for (auto &&i : util::range_of(pagelets)) {
        auto &&pagelet = pagelets[i];
        auto &&pagelet_start_offset = pagelet_offsets[i];
        auto &&pagelet_end_offset = pagelet_offsets[i + 1];

        // Ensure the pagelet data doesn't overlap the page header.
        if (pagelet_start_offset < 20 + pagelet_count * 4)
            throw res::import_error("nsf::spage: pagelet out of bounds");

        // Ensure the pagelet data doesn't extend past the end of the
        // page (overrun).
        if (pagelet_end_offset > page_size)
            throw res::import_error("nsf::spage: pagelet too long");

        // Ensure the pagelet doesn't end before it begins (negative
        // pagelet size).
        if (pagelet_end_offset < pagelet_start_offset)
            throw res::import_error("nsf::spage: negative pagelet size");

        // Create the pagelet asset.
        pagelet = get_name() / "pagelet-$"_fmt(i);
        pagelet.create(TS, get_proj());

        // Copy the pagelet data into the asset.
        pagelet->set_data(TS,
            {&data[pagelet_start_offset], &data[pagelet_end_offset]}
        );
    }

    // Finish importing.
    set_type(TS, type);
    set_cid(TS, cid);
    set_checksum(TS, checksum);
    set_pagelets(TS, {pagelets.begin(), pagelets.end()});
}

// declared in nsf.hh
util::blob spage::export_file() const
{
    assert_alive();

    util::binwriter w;
    w.begin();

    auto &&pagelets = get_pagelets();

    // Write the page header.
    w.write_u16(0x1234);
    w.write_u16(get_type());
    w.write_u32(get_cid());
    w.write_u32(pagelets.size());
    w.write_u32(get_checksum());

    // Export the pagelets if they are processed entries.
    std::vector<util::blob> pagelets_raw(pagelets.size());
    for (auto &&i : util::range_of(get_pagelets())) {
        auto ref = get_pagelets()[i];

        if (!ref)
            throw res::export_error("nsf::spage: null pagelet ref");

        misc::raw_data::ref raw_ref = ref;
        if (raw_ref.ok()) {
            pagelets_raw[i] = raw_ref->get_data();
            continue;
        }

        entry::ref entry_ref = ref;
        if (entry_ref.ok()) {
            pagelets_raw[i] = entry_ref->export_file();
            continue;
        }

        throw res::export_error("nsf::spage: pagelet has incompatible type");
    }

    // Calculate and write the pagelet offsets.
    std::uint32_t pagelet_offset = 20 + pagelets.size() * 4;
    for (auto &&pagelet : pagelets_raw) {
        w.write_u32(pagelet_offset);
        pagelet_offset += pagelet.size();
    }
    w.write_u32(pagelet_offset);

    auto data = w.end();

    // Write the pagelets themselves.
    for (auto &&pagelet : pagelets_raw) {
        data.insert(data.end(), pagelet.begin(), pagelet.end());
    }

    // Ensure a 64K page size.
    if (data.size() > page_size)
        throw res::export_error("nsf::spage: over 64K page size");

    if (data.size() < page_size) {
        data.resize(page_size);
    }

    // Calculate checksum, if necessary.
    // TODO

    return data;
}

}
}
