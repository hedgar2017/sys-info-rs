extern crate sys_info;

use sys_info::*;

fn main() {
    println!("OS type           : {}", os_type().unwrap_or("Error".to_owned()));
    println!("OS release        : {}", os_release().unwrap_or("Error".to_owned()));

    println!();
    println!("System uptime     : {} seconds", uptime().unwrap_or(-1));
    println!("Hostname          : {}", hostname().unwrap_or("Error".to_owned()));

    println!();
    println!("CPU cores count   : {}", cpu_core_count().unwrap_or(-1));
    println!("CPU frequency     : {} MHz", cpu_speed().unwrap_or(-1));
    let load = cpu_load_average().unwrap_or(LoadAverage {one: 0.0, five: 0.0, fifteen: 0.0});
    println!("CPU load average  : {:.2} {:.2} {:.2}", load.one, load.five, load.fifteen);
    println!("Process count     : {}", process_count().unwrap_or(-1));

    println!();
    let memory = memory_info().unwrap_or(MemoryInfo {free: 0, total: 0});
    println!("Memory info       : free {} bytes, total {} bytes", memory.free, memory.total);
    let swap = swap_info().unwrap_or(SwapInfo {free: 0, total: 0});
    println!("Swap info         : free {} bytes, total {} bytes", swap.free, swap.total);
}