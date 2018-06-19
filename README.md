# sys-info-rs

Get system information in Rust.

For now it supports Linux, Mac OS X, Windows and FreeBSD.
And now it can get information of kernel/cpu/memory/load/hostname and so on.

### Usage
Add this to `Cargo.toml`:

```
[dependencies]
sys-info = "*"
```

and add this to crate root:

```
extern crate sys_info;
```