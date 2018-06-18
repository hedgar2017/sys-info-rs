//!
//! This crate focuses on getting system information.
//! For now it supports Linux, Android, Windows and FreeBSD.
//!

extern crate libc;
#[macro_use] extern crate failure;

use std::{
    os::raw::c_char,
    ffi::CStr,
};

#[repr(C)]
#[derive(Debug)]
pub struct LoadAverage {
    pub one: f64,
    pub five: f64,
    pub fifteen: f64,
}

#[repr(C)]
#[derive(Debug)]
pub struct MemoryInfo {
    pub free: u64,
    pub total: u64,
}

#[repr(C)]
#[derive(Debug)]
pub struct SwapInfo {
    pub free: u64,
    pub total: u64,
}

#[repr(C)]
#[derive(Debug)]
pub struct DiskInfo {
    pub free: u64,
    pub total: u64,
}

#[derive(Fail, Debug)]
pub enum Error {
    #[fail(display = "Unsupported system")]
    UnsupportedSystem,
    #[fail(display = "System code: {}", _0)]
    SystemCode(i32),
}

macro_rules! error_if_unsupported {
    () => {
        if
            !cfg!(target_os = "linux") &&
            !cfg!(target_os = "android") &&
            !cfg!(target_os = "windows") &&
            !cfg!(target_os = "freebsd") &&
            !cfg!(target_os = "macos") &&
            !cfg!(target_os = "ios")
        {
            return Err(Error::UnsupportedSystem);
        }
    }
}

extern "C" {
    fn get_os_type(buf: *mut i8, size: usize) -> i32;
    fn get_os_release(buf: *mut i8, size: usize) -> i32;

    fn get_uptime(value: *mut i32) -> i32;
    fn get_hostname(buf: *mut i8, size: usize) -> i32;

    fn get_cpu_core_count(value: *mut i32) -> i32;
    fn get_cpu_speed(value: *mut i32) -> i32;
    fn get_cpu_load_average(value: &LoadAverage) -> i32;
    fn get_process_count(value: *mut i32) -> i32;

    fn get_memory_info(value: &MemoryInfo) -> i32;
    fn get_swap_info(value: &SwapInfo) -> i32;
    fn get_disk_info(value: &DiskInfo) -> i32;
}

pub fn os_type() -> Result<String, Error> {
    error_if_unsupported!();

    const SIZE: usize = 256;
    let mut buf = [0i8; SIZE];
    let result = unsafe { get_os_type(&mut buf[0] as *mut i8, SIZE) };
    if result != 0 {
        return Err(Error::SystemCode(result));
    }
    let result = unsafe { CStr::from_ptr(&buf[0] as *const c_char).to_bytes() };
    let result = String::from_utf8_lossy(result).into_owned();
    Ok(result)
}

pub fn os_release() -> Result<String, Error> {
    error_if_unsupported!();

    const SIZE: usize = 256;
    let mut buf = [0i8; SIZE];
    let result = unsafe { get_os_release(&mut buf[0] as *mut i8, SIZE) };
    if result != 0 {
        return Err(Error::SystemCode(result));
    }
    let result = unsafe { CStr::from_ptr(&buf[0] as *const c_char).to_bytes() };
    let result = String::from_utf8_lossy(result).into_owned();
    Ok(result)
}

pub fn uptime() -> Result<i32, Error> {
    error_if_unsupported!();

    let mut value = 0;
    let result = unsafe { get_uptime(&mut value as *mut i32) };
    if result != 0 {
        return Err(Error::SystemCode(result));
    }
    Ok(value)
}

pub fn hostname() -> Result<String, Error> {
    error_if_unsupported!();

    const SIZE: usize = 256;
    let mut buf = [0i8; SIZE];
    let result = unsafe { get_hostname(&mut buf[0] as *mut i8, SIZE) };
    if result != 0 {
        return Err(Error::SystemCode(result));
    }
    let result = unsafe { CStr::from_ptr(&buf[0] as *const c_char).to_bytes() };
    let result = String::from_utf8_lossy(result).into_owned();
    Ok(result)
}

/// Notice, it returns the logical cpu quantity.
pub fn cpu_core_count() -> Result<i32, Error> {
    error_if_unsupported!();

    let mut value = 0;
    let result = unsafe { get_cpu_core_count(&mut value as *mut i32) };
    if result != 0 {
        return Err(Error::SystemCode(result));
    }
    Ok(value)
}

/// Get cpu speed in MHz.
pub fn cpu_speed() -> Result<i32, Error> {
    error_if_unsupported!();

    let mut value = 0;
    let result = unsafe { get_cpu_speed(&mut value as *mut i32) };
    if result != 0 {
        return Err(Error::SystemCode(result));
    }
    Ok(value)
}

/// Notice, on windows, one/five/fifteen of the LoadAverage returned are the current load in %%.
pub fn cpu_load_average() -> Result<LoadAverage, Error> {
    error_if_unsupported!();

    let load = LoadAverage {one: 0.0, five: 0.0, fifteen: 0.0};
    let result = unsafe { get_cpu_load_average(&load) };
    if result != 0 {
        return Err(Error::SystemCode(result));
    }
    Ok(load)
}

pub fn process_count() -> Result<i32, Error> {
    error_if_unsupported!();

    let mut value = 0;
    let result = unsafe { get_process_count(&mut value as *mut i32) };
    if result != 0 {
        return Err(Error::SystemCode(result));
    }
    Ok(value)
}

pub fn memory_info() -> Result<MemoryInfo, Error> {
    error_if_unsupported!();

    let info = MemoryInfo {free: 0, total: 0};
    let result = unsafe { get_memory_info(&info) };
    if result != 0 {
        return Err(Error::SystemCode(result));
    }
    Ok(info)
}

pub fn swap_info() -> Result<SwapInfo, Error> {
    error_if_unsupported!();

    let info = SwapInfo {free: 0, total: 0};
    let result = unsafe { get_swap_info(&info) };
    if result != 0 {
        return Err(Error::SystemCode(result));
    }
    Ok(info)
}

pub fn disk_info() -> Result<DiskInfo, Error> {
    error_if_unsupported!();

    let info = DiskInfo {free: 0, total: 0};
    let result = unsafe { get_disk_info(&info) };
    if result != 0 {
        return Err(Error::SystemCode(result));
    }
    Ok(info)
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    pub fn test_os_type() {
        let _ = os_type()
            .map(|value| assert!(value.len() > 0, "Empty string is very unlikely"))
            .map_err(|error| assert!(false, error.to_string()));
    }

    #[test]
    pub fn test_os_release() {
        let _ = os_release()
            .map(|value| assert!(value.len() > 0, "Empty string is very unlikely"))
            .map_err(|error| assert!(false, error.to_string()));
    }

    #[test]
    pub fn test_uptime() {
        let _ = uptime()
            .map(|value| assert!(value > 0, "Zero is very unlikely"))
            .map_err(|error| assert!(false, error.to_string()));
    }

    #[test]
    pub fn test_hostname() {
        let _ = hostname()
            .map(|value| assert!(value.len() > 0, "Empty string is very unlikely"))
            .map_err(|error| assert!(false, error.to_string()));
    }

    #[test]
    pub fn test_cpu_core_count() {
        let _ = cpu_core_count()
            .map(|value| assert!(value > 0, "Zero is very unlikely"))
            .map_err(|error| assert!(false, error.to_string()));
    }

    #[test]
    pub fn test_cpu_speed() {
        let _ = cpu_speed()
            .map(|value| assert!(value > 0, "Zero is very unlikely"))
            .map_err(|error| assert!(false, error.to_string()));
    }

    #[test]
    pub fn test_cpu_load_average() {
        let _ = cpu_load_average()
            .map(|value| {
                assert!(value.one > 0.0, "Zero is very unlikely");
                assert!(value.five > 0.0, "Zero is very unlikely");
                assert!(value.fifteen > 0.0, "Zero is very unlikely");
            })
            .map_err(|error| assert!(false, error.to_string()));
    }

    #[test]
    pub fn test_process_count() {
        let _ = process_count()
            .map(|value| assert!(value > 0, "Zero is very unlikely"))
            .map_err(|error| assert!(false, error.to_string()));
    }

    #[test]
    pub fn test_memory_info() {
        let _ = memory_info()
            .map(|value| {
                assert!(value.free > 0, "Zero is very unlikely");
                assert!(value.total > 0, "Zero is very unlikely");
                assert!(value.swap_free > 0, "Zero is very unlikely");
                assert!(value.swap_total > 0, "Zero is very unlikely");
            })
            .map_err(|error| assert!(false, error.to_string()));
    }

    #[test]
    pub fn test_swap_info() {
        let _ = swap_info()
            .map(|value| {
                assert!(value.free > 0, "Zero is very unlikely");
                assert!(value.total > 0, "Zero is very unlikely");
            })
            .map_err(|error| assert!(false, error.to_string()));
    }

    #[test]
    pub fn test_disk_info() {
        let _ = disk_info()
            .map(|value| {
                assert!(value.free > 0, "Zero is very unlikely");
                assert!(value.total > 0, "Zero is very unlikely");
            })
            .map_err(|error| assert!(false, error.to_string()));
    }
}