mod ogmo;

use ogmo::Level as OgmoLevel;
use ogmo::{BackgroundLayer, EntityLayer, LevelValues};

use clap::{App, Arg};

use std::ffi::OsStr;
use std::fs;
use std::io::{self};
use std::path::PathBuf;

use std::fmt::Write as FmtWrite;

// To parse an Ogmo level into a chunk:
//
// 1) Process "values" from the [Ogmo]Level - done automatically with serde.
// 2) Process the "background" TileLayer
// 3) Process the "enemies" layer,
//  taking care to adapt the coordinates (must be divided by 8),
//  and overwriting preexisting tiles.

fn to_tiles(lvl: &OgmoLevel) -> Vec<i32> {
    // I tried to use tuple destructuring here, but the refs made my life too difficult...
    let bg: &BackgroundLayer = &lvl.layers.0;

    let mut tile_ids = bg.data.clone();

    let ent_layer: &EntityLayer = &lvl.layers.1;

    for entity in &ent_layer.entities {
        let (i, j) = (entity.x / 8, entity.y / 8);

        // Printing to stderr allows use to reserve stdout for the binary output.
        eprintln!(
            "Found entity with id {} at coords ({}, {})",
            entity.values.id, i, j
        );
        let tile_idx = j * 8 + i;
        eprintln!("Writing to tile index {}", tile_idx);
        tile_ids[(tile_idx) as usize] = entity.values.id as i32;
    }

    tile_ids
}

fn fits_in_n_ubits(tile_id: i32, n: u8) -> bool {
    0 <= tile_id && tile_id < (1 << n)
}

fn to_binary(tile_ids: &[i32], values: LevelValues) -> [u8; 33] {
    let mut binary_buffer = [0u8; 33];

    // Handle the first 32 bytes: tiles.
    let mut i = 0;
    for tile_pair in tile_ids.chunks(2) {
        if let &[tile_1, tile_2] = tile_pair {
            eprintln!("Handling tiles: {}, {}", tile_1, tile_2);

            assert!(fits_in_n_ubits(tile_1, 4));
            assert!(fits_in_n_ubits(tile_2, 4));

            binary_buffer[i] = ((tile_1 as u8) << 4) | (tile_2 as u8);
            i += 1;
        } else {
            // 64 is an even number, so...
            unreachable!()
        }
    }

    // Last byte: flags and clutter level.
    binary_buffer[32] =
        (values.top_door as u8) << 7 | (values.left_door as u8) << 6 | (values.corner as u8) << 5;

    assert!(fits_in_n_ubits(values.clutter_level as i32, 5));
    binary_buffer[32] |= values.clutter_level as u8;

    binary_buffer
}

fn header(chunk_count: usize) -> String {
    // XXX: I feel like we should include types.h, but img2gb doesn't do it.
    format!(
        "// This file was generated by podpacker, DO NOT EDIT

#ifndef _CHUNKS_H
#define _CHUNKS_H

extern const UINT8 CHUNKS[][];
#define CHUNK_COUNT {}


#endif
",
        chunk_count
    )
}

fn array_decl(hex_lvls: Vec<String>) -> String {
    let mut inner = String::new();

    for s in hex_lvls {
        // '{{' is how you escape a curly bracket in a format string.
        write!(inner, "{{ {} }},\n", s).unwrap();
    }

    format!(
        "// This file was auto-generated by podpacker. DO NOT EDIT.
#include <types.h>

const UINT8 CHUNKS[][] = {{ {} }};",
        inner
    )
}

fn main() -> io::Result<()> {
    let matches = App::new("podpacker - Pineapple of Doom Map Packer")
        .version("0.1")
        .author("Yohaï-Eliel BERREBY <yohaiberreby@gmail.com>")
        .about(
            "Converts Ogmo Map Editor 3 JSON levels to Pineapple of Doom's packed\
            binary representation.",
        )
        .arg(
            Arg::with_name("JSON_DIR")
                .help("Sets the directory from which to extract levels")
                .required(true)
                .index(1),
        )
        .arg(
            Arg::with_name("OUTPUT_DIR")
                .help("Sets the directory where to write the .c and .h (default: .)")
                .required(false)
                .index(2),
        )
        .arg(
            Arg::with_name("output-format")
                .help("Whether to output the binary data as-is, byte by byte, or generate a C file")
                .long("output-format")
                .possible_values(&["raw", "c"])
                .takes_value(true),
        )
        .get_matches();

    let search_dir = matches.value_of("JSON_DIR").unwrap();

    let mut lvls = Vec::new();

    for entry in fs::read_dir(search_dir)? {
        let entry = entry?;
        let path = entry.path();
        if path.extension() == Some(OsStr::new("json")) {
            let s = fs::read_to_string(path)?;
            let level: OgmoLevel = serde_json::from_str(&s).unwrap();
            lvls.push(level);
        }
    }

    // In-place sort.
    lvls.sort_by(|a, b| {
        a.values
            .clutter_level
            .partial_cmp(&b.values.clutter_level)
            .unwrap()
    });

    let bin_lvls: Vec<_> = lvls
        .iter()
        .map(|lvl| {
            let tiles = to_tiles(&lvl);
            to_binary(&tiles, lvl.values)
        })
        .collect();

    let hex_lvls: Vec<String> = bin_lvls
        .into_iter()
        .map(|bin_buf| {
            bin_buf
                .iter()
                .map(|byte| format!("0x{:X?}", byte))
                .collect::<Vec<_>>()
                .join(", ")
        })
        .collect();

    let output_dir = matches.value_of("OUTPUT_DIR").unwrap_or(".");
    let mut path = PathBuf::new();
    path.push(output_dir);
    path.push("chunks");

    path.set_extension("h");
    fs::write(&path, header(hex_lvls.len()))?;

    path.set_extension("c");
    fs::write(&path, array_decl(hex_lvls))?;

    Ok(())
}
