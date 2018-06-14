//! #Introduction
//! This crate focuses on geting system information.
//!
//! For now it supports Linux, Mac OS X and Windows.
//! And now it can get information of kernel/cpu/memory/disk/load/hostname and so on.
//!

extern crate libc;

use std::ffi;
use std::fmt;
use std::io::{self, Read};
use std::fs::File;
use std::num::{ParseIntError, ParseFloatError};
use std::os::raw::c_char;
use std::collections::HashMap;

#[repr(C)]
#[derive(Debug)]
pub struct LoadAvg {
    pub one: f64,
    pub five: f64,
    pub fifteen: f64,
}

#[repr(C)]
#[derive(Debug)]
pub struct MemInfo {
    pub total: u64,
    pub free: u64,
    pub avail: u64,

    pub buffers: u64,
    pub cached: u64,

    pub swap_total: u64,
    pub swap_free: u64,
}

#[repr(C)]
#[derive(Debug)]
pub struct DiskInfo {
    pub total: u64,
    pub free: u64,
}

#[derive(Debug)]
pub enum Error {
    UnsupportedSystem,
    ExecFailed(io::Error),
    IO(io::Error),
    ParsingInt(ParseIntError),
    ParsingFloat(ParseFloatError),
    Unknown,
}

impl fmt::Display for Error {
    fn fmt(&self, fmt: &mut fmt::Formatter) -> fmt::Result {
        use self::Error::*;
        match *self {
            UnsupportedSystem => write!(fmt, "System is not supported"),
            ExecFailed(ref e) => write!(fmt, "Execution failed: {}", e),
            IO(ref e) => write!(fmt, "I/O error: {}", e),
            ParsingInt(ref e) => write!(fmt, "Integer parsing error: {}", e),
            ParsingFloat(ref e) => write!(fmt, "Floating point parsing error: {}", e),
            Unknown => write!(fmt, "An unknown error occurred"),
        }
    }
}

impl std::error::Error for Error {
    fn description(&self) -> &str {
        use self::Error::*;
        match *self {
            UnsupportedSystem => "Unsupported system",
            ExecFailed(_) => "Execution failed",
            IO(_) => "I/O error",
            ParsingInt(_) => "Integer parsing error",
            ParsingFloat(_) => "Floating point parsing error",
            Unknown => "Unknown error",
        }
    }

    fn cause(&self) -> Option<&std::error::Error> {
        use self::Error::*;
        match *self {
            UnsupportedSystem => None,
            ExecFailed(ref e) => Some(e),
            IO(ref e) => Some(e),
            ParsingInt(ref e) => Some(e),
            ParsingFloat(ref e) => Some(e),
            Unknown => None,
        }
    }
}

impl From<io::Error> for Error {
    fn from(e: io::Error) -> Error {
        Error::IO(e)
    }
}

impl From<ParseIntError> for Error {
    fn from(e: ParseIntError) -> Error {
        Error::ParsingInt(e)
    }
}

impl From<ParseFloatError> for Error {
    fn from(e: ParseFloatError) -> Error {
        Error::ParsingFloat(e)
    }
}

extern "C" {
    fn get_os_type() -> *const i8;
    fn get_os_release() -> *const i8;

    fn get_cpu_num() -> u32;
    fn get_cpu_speed() -> u64;

    fn get_loadavg() -> LoadAvg;
    fn get_proc_total() -> u64;

    fn get_mem_info() -> MemInfo;
    fn get_disk_info() -> DiskInfo;

    fn get_uptime() -> f64;
}

/// Get operation system type.
///
/// Such as "Linux", "Darwin", "Windows".
pub fn os_type() -> Result<String, Error> {
    if cfg!(target_os = "linux") || cfg!(target_os = "android") {
        let mut s = String::new();
        File::open("/proc/sys/kernel/ostype")?.read_to_string(&mut s)?;
        s.pop(); // pop '\n'
        Ok(s)
    } else if cfg!(target_os = "macos") || cfg!(target_os = "ios") || cfg!(target_os = "windows") {
        let typ = unsafe { ffi::CStr::from_ptr(get_os_type() as *const c_char).to_bytes() };
        Ok(String::from_utf8_lossy(typ).into_owned())
    } else {
        Err(Error::UnsupportedSystem)
    }
}

/// Get operation system release version.
///
/// Such as "3.19.0-gentoo"
pub fn os_release() -> Result<String, Error> {
    if cfg!(target_os = "linux") || cfg!(target_os = "android") {
        let mut s = String::new();
        File::open("/proc/sys/kernel/osrelease")?.read_to_string(&mut s)?;
        s.pop(); // pop '\n'
        Ok(s)
    } else if cfg!(target_os = "macos") || cfg!(target_os = "ios") || cfg!(target_os = "windows") {
        let typ = unsafe { ffi::CStr::from_ptr(get_os_release() as *const c_char).to_bytes() };
        Ok(String::from_utf8_lossy(typ).into_owned())
    } else {
        Err(Error::UnsupportedSystem)
    }
}

/// Get cpu num quantity.
///
/// Notice, it returns the logical cpu quantity.
pub fn cpu_num() -> Result<u32, Error> {
    if cfg!(unix) || cfg!(windows) {
        unsafe { Ok(get_cpu_num()) }
    } else {
        Err(Error::UnsupportedSystem)
    }
}

/// Get cpu speed.
///
/// Such as 2500, that is 2500 MHz.
pub fn cpu_speed() -> Result<u64, Error> {
    if cfg!(target_os = "linux") || cfg!(target_os = "android") {
        // /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq
        let mut s = String::new();
        File::open("/proc/cpuinfo")?.read_to_string(&mut s)?;

        let mut find_cpu_mhz = s.split('\n').find(|line| line.starts_with("cpu MHz"));
        match find_cpu_mhz {
            None => find_cpu_mhz = s.split('\n').find(|line| line.starts_with("BogoMIPS")),
            _ => {}
        }

        find_cpu_mhz.and_then(|line| line.split(':').last())
            .and_then(|val| val.trim().parse::<f64>().ok())
            .map(|speed| speed as u64)
            .ok_or(Error::Unknown)

    } else if cfg!(target_os = "macos") || cfg!(target_os = "ios") || cfg!(target_os = "windows") {
        unsafe { Ok(get_cpu_speed()) }
    } else {
        Err(Error::UnsupportedSystem)
    }
}

/// Get system load average value.
///
/// Notice, on windows, one/five/fifteen of the LoadAvg returned are the current load.
pub fn loadavg() -> Result<LoadAvg, Error> {
    if cfg!(target_os = "linux") || cfg!(target_os = "android") {
        let mut s = String::new();
        File::open("/proc/loadavg")?.read_to_string(&mut s)?;
        let loads = s.trim().split(' ')
            .take(3)
            .map(|val| val.parse::<f64>().unwrap())
            .collect::<Vec<f64>>();
        Ok(LoadAvg {
            one: loads[0],
            five: loads[1],
            fifteen: loads[2],
        })
    } else if cfg!(target_os = "macos") || cfg!(target_os = "ios") || cfg!(target_os = "windows") {
        Ok(unsafe { get_loadavg() })
    } else {
        Err(Error::UnsupportedSystem)
    }
}

/// Get current processes quantity.
///
/// Notice, it temporarily does not support Windows.
pub fn proc_total() -> Result<u64, Error> {
    if cfg!(target_os = "linux") || cfg!(target_os = "android") {
        let mut s = String::new();
        File::open("/proc/loadavg")?.read_to_string(&mut s)?;
        s.split(' ')
            .nth(3)
            .and_then(|val| val.split('/').last())
            .and_then(|val| val.parse::<u64>().ok())
            .ok_or(Error::Unknown)
    } else if cfg!(target_os = "macos") || cfg!(target_os = "ios") || cfg!(target_os = "windows") {
        Ok(unsafe { get_proc_total() })
    } else {
        Err(Error::UnsupportedSystem)
    }
}

/// Get memory information.
///
/// On Mac OS X and Windows, the buffers and cached variables of the MemInfo returned are zero.
pub fn mem_info() -> Result<MemInfo, Error> {
    if cfg!(target_os = "linux") || cfg!(target_os = "android") {
        let mut s = String::new();
        File::open("/proc/meminfo")?.read_to_string(&mut s)?;
        let mut meminfo_hashmap = HashMap::new();
        for line in s.lines() {
            let mut split_line = line.split_whitespace();
            let label = split_line.next();
            let value = split_line.next();
            if value.is_some() && label.is_some() {
                let label = label.unwrap().split(':').nth(0).ok_or(Error::Unknown)?;
                let value = value.unwrap().parse::<u64>().ok().ok_or(Error::Unknown)?;
                meminfo_hashmap.insert(label, value);
            }
        }
        Ok(MemInfo {
            total: *meminfo_hashmap.get("MemTotal").ok_or(Error::Unknown)?,
            free: *meminfo_hashmap.get("MemFree").ok_or(Error::Unknown)?,
            avail: *meminfo_hashmap.get("MemAvailable").ok_or(Error::Unknown)?,
            buffers: *meminfo_hashmap.get("Buffers").ok_or(Error::Unknown)?,
            cached: *meminfo_hashmap.get("Cached").ok_or(Error::Unknown)?,
            swap_total: *meminfo_hashmap.get("SwapTotal").ok_or(Error::Unknown)?,
            swap_free: *meminfo_hashmap.get("SwapFree").ok_or(Error::Unknown)?,
        })
    } else if cfg!(target_os = "macos") || cfg!(target_os = "ios") || cfg!(target_os = "windows") {
        Ok(unsafe { get_mem_info() })
    } else {
        Err(Error::UnsupportedSystem)
    }
}

/// Get disk information.
///
/// Notice, it just calculate current disk on Windows.
pub fn disk_info() -> Result<DiskInfo, Error> {
    if cfg!(target_os = "linux") || cfg!(target_os = "android") || cfg!(target_os = "macos") || cfg!(target_os = "ios") || cfg!(target_os = "windows") {
        Ok(unsafe { get_disk_info() })
    } else {
        Err(Error::UnsupportedSystem)
    }
}

/// Get hostname.
pub fn hostname() -> Result<String, Error> {
    use std::process::Command;
    Command::new("hostname")
        .output()
        .map_err(Error::ExecFailed)
        .map(|output| String::from_utf8(output.stdout).unwrap().trim().to_string())
}

/// Get system uptime
pub fn uptime() -> Result<f64, Error> {
    if cfg!(target_os = "linux") || cfg!(target_os = "android") {
        let mut s = String::new();
        File::open("/proc/uptime")?.read_to_string(&mut s)?;
        let secs = s.trim().split(' ')
            .collect::<Vec<&str>>()
            .get(0)
            .ok_or(Error::Unknown)?
            .parse::<f64>()?;
        Ok(secs)
    } else if cfg!(target_os = "windows") || cfg!(target_os = "macos") || cfg!(target_os = "ios") {
        Ok(unsafe { get_uptime() })
    } else {
        Err(Error::UnsupportedSystem)
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    pub fn test_os_type() {
        let typ = os_type().unwrap();
        assert!(typ.len() > 0);
        println!("os_type()         : {}", typ);
    }

    #[test]
    pub fn test_os_release() {
        let release = os_release().unwrap();
        assert!(release.len() > 0);
        println!("os_release()      : {}", release);
    }

    #[test]
    pub fn test_cpu_num() {
        let num = cpu_num().unwrap();
        assert!(num > 0);
        println!("cpu_num()         : {}", num);
    }

    #[test]
    pub fn test_cpu_speed() {
        let speed = cpu_speed().unwrap();
        assert!(speed > 0);
        println!("cpu_speed()       : {}", speed);
    }

    #[test]
    pub fn test_loadavg() {
        let load = loadavg().unwrap();
        println!("loadavg()         : {:?}", load);
    }

    #[test]
    pub fn test_proc_total() {
        let procs = proc_total().unwrap();
        assert!(procs > 0);
        println!("proc_total()      : {}", procs);
    }

    #[test]
    pub fn test_mem_info() {
        let mem = mem_info().unwrap();
        assert!(mem.total > 0);
        println!("mem_info()        : {:?}", mem);
    }

    #[test]
    pub fn test_disk_info() {
        let info = disk_info().unwrap();
        println!("disk_info()       : {:?}", info);
    }

    #[test]
    pub fn test_hostname() {
        let host = hostname().unwrap();
        assert!(host.len() > 0);
        println!("hostname()        : {}", host);
    }

    #[test]
    pub fn test_uptime() {
        let uptime = uptime().unwrap();
        println!("uptime()          : {}", uptime);
        assert!(uptime > 0);
    }
}