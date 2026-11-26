# Preview pagination

The preview grid is 16 steps, one pad row. Page count is `ceil(requested_length / 16)`.

| Requested length | Pages |
|---|---|
| 16 | 1 |
| 32 | 2 |
| 64 | 4 |
| 128 | Not offered. Native length cannot be 128. |

While the preview or the replace confirm is up, an encoder turn changes the page. Shift+turn still changes the parameter only on the config screen. Pad LEDs use `page * 16 + column`.

The config row shows track, requested `len`, and native `pat`. The preview row shows track, `pat`, `pg`, and `/` pages, plus the event count and the seed. Apply uses `g->events[]`, the full stored preview, including steps that are not on the visible page.

Changing lab, track, length, or any other parameter invalidates the preview. A native occupancy change on any step of the destination, including step 20 while page 1 is showing, makes Apply report “Stale preview” and drop the preview.

Screens:

- `screenshots/config_pat64_len16.png` — track 9, requested 16, pattern 64.
- `screenshots/preview_mel32_p2.png` — page 2/2, seed 42, 25 events.
- `screenshots/preview_walk64_p4.png` — page 4/4, seed 2026, pattern 64.
- `screenshots/preview_arp64_p1.png` through `p4.png` — four pages, then a full-pattern readback.

Default requested length remains 16 on a 32-step and on a 64-step track, so an untouched Apply still fills only the first page’s worth of steps the user asked for.
