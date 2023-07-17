#![no_std]
#![feature(alloc_error_handler)]

#[panic_handler]
fn panic(info: &core::panic::PanicInfo) -> ! { mcu_if::panic(info) }

#[alloc_error_handler]
fn alloc_error(layout: mcu_if::alloc::alloc::Layout) -> ! { mcu_if::alloc_error(layout) }

use core::cell::{Cell, RefCell};
use core::future::Future;
use mcu_if::{println, alloc::rc::Rc};
use async_task::{Runnable, Task};
use crossbeam_queue::ArrayQueue;

#[no_mangle]
pub extern fn rustmod_start() {
    println!("[src/lib.rs] rustmod_start(): ^^");
    rustmod_tests();
}

async fn inc(val: Rc<Cell<u8>>) -> Result<bool, ()>{
    println!("@@ inc() before: val: {}", val.get());
    val.set(val.get() + 1);
    println!("@@ inc() after: val: {}", val.get());
    if 0 == 1 { loop {} } // debug

    Ok(true)
}

fn rustmod_tests() {
    println!("@@ rustmod_tests(): ^^");

    let val = Rc::new(Cell::new(0));
    println!("@@ rustmod_tests(): val: {}", val.get());

    let rt0 = Rc::new(Runtime::new());
    {
        let val = val.clone();
        let rt = rt0.clone();

        rt0.spawn_local(async move {
            println!("@@ future0: ^^ val: {}", val.get());

            //

            val.set(val.get() + 1);

            //

            let ret = rt.exec(inc(val.clone())).await;
            println!("@@ ret: {:?}", ret);

            //

            let fut = {
                let val = val.clone();
                async move {
                    println!("@@ future1: ^^ val: {}", val.get());
                    val.set(val.get() + 1);
                    println!("@@ future1: $$ val: {}", val.get());
                }
            };
            rt.exec(fut).await;

            //

            println!("@@ future0: $$ val: {}", val.get());
        });
    }

    println!("@@ rustmod_tests(): $$ val: {}", val.get());
}

//----@@ adaptation of https://github.com/smol-rs/async-task/blob/9ff587ecab7b9a9fa81672f4dbf315ff375b6e5e/examples/spawn-local.rs#L51
const RUNTIME_CAP_DEFAULT: usize = 16;
struct Runtime(Rc<RefCell<ArrayQueue<Runnable>>>);
impl Runtime {
    pub fn new() -> Self {
        Self(Rc::new(RefCell::new(
            crossbeam_queue::ArrayQueue::<Runnable>::new(RUNTIME_CAP_DEFAULT))))
    }

    /// Spawns a future on the executor.
    pub fn exec<F, T>(&self, future: F) -> Task<T>
    where
        F: Future<Output = T> + 'static,
        T: 'static,
    {
        // Create a task that is scheduled by pushing itself into the queue.
        let schedule = |runnable| self.0.borrow().push(runnable).unwrap();
        let (runnable, task) = unsafe { async_task::spawn_unchecked(future, schedule) };

        // Schedule the task by pushing it into the queue.
        runnable.schedule();

        task
    }

    pub fn spawn_local<F, T>(&self, future: F) -> T
    where
        F: Future<Output = T> + 'static,
        T: 'static,
    {
        // Spawn a task that sends its result through a channel.
        let oneshot = Rc::new(RefCell::new(crossbeam_queue::ArrayQueue::new(1)));
        let oneshot_cloned = oneshot.clone();
        self.exec(async move {
            println!("@@ future-spawn-local: ^^");
            drop(oneshot.borrow().push(future.await))
        }).detach();

        loop {
            println!("@@ loop: ^^");
            // If the original task has completed, return its result.
            if let Some(val) = oneshot_cloned.borrow().pop() {
                return val;
            }
            println!("@@ loop: --");

            // Otherwise, take a task from the queue and run it.
            self.0.borrow().pop().unwrap().run();
            println!("@@ loop: $$");
        }
    }
}