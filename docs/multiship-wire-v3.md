# MultiShip wire protocol — 'Start Multiworld Save' + v3 SeedData (F-035 Part A)

This is the server→client contract for handing a player the multiworld seed. It is the
authoritative spec the SoH client (ShipwreckCombo, Part B — a separate repo) implements
the matching request + deserializer against.

Scope of Part A: **transport + world lock only.** No generator changes, no `.multiship`
format change. The bytes shipped over the wire are produced by the SAME serializer that
writes the `.multiship` file (`SeedFile::SerializeToBytes`, sharing `SerializeV3` with
`WriteMultiship`), so **file == wire, byte for byte**.

## Transport (unchanged)

Same as the rest of the MultiShip relay: a raw TCP connection carrying **NUL-delimited
(`'\0'`) JSON packets** (SDL2_net, mirroring the SoH `Network` client). Each packet is one
UTF-8 JSON object terminated by a single `'\0'`.

Because the v3 SeedData is **raw binary and contains `'\0'` bytes**, it cannot ride the
NUL-delimited channel directly. So the seed bytes are **base64-encoded** (RFC 4648, `+`/`/`
alphabet, `=` padding) into a JSON string field. The client base64-decodes that field back
to the byte-identical v3 bytes, then runs the v3 deserializer below. (Base64 is a transport
envelope only — it is NOT part of the seed format.)

## Messages

### 0. Server → Client: seed info (non-locking, informational)

Pushed **unsolicited** by the server right after a client connects (and re-pushed to all
connected clients whenever a seed is loaded/replaced via `SetSeed`). It carries only the
world-name list so the client can validate the player's chosen name *before* claiming a
world. It does **not** lock anything and does **not** carry the placements/settings.

```json
{ "type": "multiworld_seed_info", "seedId": "heron-snow-oak-d0fa", "players": ["Player 1", "Player 2"] }
```

- `players` — index = 0-based world id. The client greys out 'Start Multiworld Save'
  unless the entered name is in this list.
- Not sent if no seed is loaded yet.

### 1. Client → Server: request to claim a world

```json
{ "type": "start_multiworld_save", "playerName": "Player 1" }
```

- `playerName` — the world the client wants, matched **by name** against the loaded seed's
  player list. If omitted/empty, the server falls back to the name from the connection
  handshake (`userName`). Names are sanitized server-side (printable, ≤32 chars).

### 2. Server → Client: granted (world locked + seed pushed)

```json
{
  "type": "multiworld_seed",
  "version": 3,
  "worldId": 0,
  "playerName": "Player 1",
  "data": "<base64 of the byte-identical v3 SeedData>"
}
```

- `worldId` — the player's **0-based world index** (its position in the seed's player list).
  Convenience field; the same value is also derivable from the decoded blob.
- `version` — mirrors `SeedFile::kVersion` (currently `3`). The decoded blob also carries it.
- `data` — base64. Decode → the exact `.multiship` bytes → deserialize per the layout below.

On a granted request the server **locks** that player's world to the requesting connection.

### 3. Server → Client: denied

```json
{ "type": "multiworld_seed_denied", "playerName": "Player 1", "reason": "locked" }
```

`reason` is one of:

| reason          | meaning                                                              |
|-----------------|----------------------------------------------------------------------|
| `locked`        | that player/world is already locked to a **different** client        |
| `unknown_name`  | no player by that name exists in the loaded seed                     |
| `no_seed`       | the server has no seed loaded yet                                    |

A request from the connection that **already holds** the lock is granted again (idempotent
re-send), not denied.

## World lock semantics (in-memory; durable persistence is F-039)

- "Lock in" = assign the player's world index by name and reserve it so **no other client**
  can claim that name/world.
- The lock is keyed by player **name**, owned by the requesting connection (a per-session
  connection id).
- A second client requesting an already-locked name is refused (`reason: "locked"`).
- A lock is **released when its owning client disconnects** (so a reconnecting client can
  reclaim its world), and **all locks reset at the start of each server session** (a fresh
  `Start`, or `SetSeed` loading a different seed). Locks do **not** survive a server restart
  — durable persistence is the later F-039.

## Decoded v3 SeedData — exact binary byte layout

After base64-decoding `data`, the bytes are identical to the `.multiship` file. All integers
are **little-endian** (native x86-64; the writer does a raw `memcpy`-style `ostream::write`).
A `str` is a **u8 length prefix** followed by exactly that many raw bytes (max 255).

```
offset (relative)            field
-----------------------------------------------------------------------------
+0   char[4]                 magic = "MSHP"
+4   u32                     version = 3
+8   u64                     seed            (numeric RNG seed)
+16  str                     seedId          (u8 len + bytes; human-readable id)
     u8                      numWorlds
     numWorlds × str         player name per world (world index = order)

     u32                     placementCount
     placementCount × {
         u8                  locWorld        (world the location belongs to)
         u16                 loc             (RandomizerCheck enum value)
         u8                  ownerWorld      (world that OWNS the item -> who receives it)
         u16                 item            (RandomizerGet enum value)
     }

     u16                     settingCount
     settingCount × {
         u16                 key             (RandomizerSettingKey enum value)
         u16                 value           (BASE SoH option index, verbatim)
     }
```

Notes for the client deserializer:

- Each **placement** is 6 bytes: `locWorld:u8, loc:u16, ownerWorld:u8, item:u16` (in that
  order — note `ownerWorld` sits *between* `loc` and `item`).
- `ownerWorld == locWorld` ⇒ a same-world item the client grants locally; otherwise the item
  routes cross-world to `ownerWorld`'s player.
- `key`'s enum NAME is `SeedFile::SettingKeyName(key)`; `value` is the base SoH option index
  (no subset remapping), apply it verbatim.
- The on-disk reader (`SeedFile::ReadMultiship`) also still accepts legacy `version == 2`
  (no `seedId`, a flat per-RSK settings byte block that it skips). The **wire** only ever
  emits the current `version` (3).

This layout is the single source of truth shared with `SeedFile.h` /
`docs/multiship-generator-foundation.md`; if `SerializeV3`/`kVersion` change, update all three.
```
