(module
  (type (;0;) (func (param i32) (result i32)))
  (type (;1;) (func (param i32 i32) (result i32)))
  (type (;2;) (func (result i32)))
  (type (;3;) (func))
  (type (;4;) (func (param i32)))
  (type (;5;) (func (param i32 i32 i32) (result i32)))
  (type (;6;) (func (param i32 i64 i32) (result i64)))
  (import "wasi_snapshot_preview1" "args_sizes_get" (func (;0;) (type 1)))
  (import "wasi_snapshot_preview1" "args_get" (func (;1;) (type 1)))
  (import "wasi_snapshot_preview1" "proc_exit" (func (;2;) (type 4)))
  (import "env" "main" (func (;3;) (type 1)))
  (func (;4;) (type 3)
    nop)
  (func (;5;) (type 1) (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.add)
  (func (;6;) (type 3)
    call 7
    call 2
    unreachable)
  (func (;7;) (type 2) (result i32)
    (local i32 i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 0
    global.set 0
    block  ;; label = @1
      local.get 0
      local.tee 2
      i32.const 12
      i32.add
      local.get 0
      i32.const 8
      i32.add
      call 0
      i32.eqz
      if  ;; label = @2
        block (result i32)  ;; label = @3
          i32.const 0
          local.get 2
          i32.load offset=12
          local.tee 1
          i32.eqz
          br_if 0 (;@3;)
          drop
          local.get 0
          local.get 1
          i32.const 2
          i32.shl
          local.tee 3
          i32.const 19
          i32.add
          i32.const -16
          i32.and
          i32.sub
          local.tee 0
          local.tee 1
          global.set 0
          local.get 1
          local.get 2
          i32.load offset=8
          i32.const 15
          i32.add
          i32.const -16
          i32.and
          i32.sub
          local.tee 1
          global.set 0
          local.get 0
          local.get 3
          i32.add
          i32.const 0
          i32.store
          local.get 0
          local.get 1
          call 1
          br_if 2 (;@1;)
          local.get 2
          i32.load offset=12
        end
        local.get 0
        call 3
        local.set 0
        local.get 2
        i32.const 16
        i32.add
        global.set 0
        local.get 0
        return
      end
      i32.const 71
      call 2
      unreachable
    end
    i32.const 71
    call 2
    unreachable)
  (func (;8;) (type 2) (result i32)
    i32.const 1024)
  (func (;9;) (type 2) (result i32)
    global.get 0)
  (func (;10;) (type 4) (param i32)
    local.get 0
    global.set 0)
  (func (;11;) (type 0) (param i32) (result i32)
    global.get 0
    local.get 0
    i32.sub
    i32.const -16
    i32.and
    local.tee 0
    global.set 0
    local.get 0)
  (func (;12;) (type 0) (param i32) (result i32)
    (local i32)
    local.get 0
    if  ;; label = @1
      local.get 0
      i32.load offset=76
      i32.const -1
      i32.le_s
      if  ;; label = @2
        local.get 0
        call 13
        return
      end
      local.get 0
      call 13
      return
    end
    i32.const 1040
    i32.load
    if  ;; label = @1
      i32.const 1040
      i32.load
      call 12
      local.set 1
    end
    i32.const 1036
    i32.load
    local.tee 0
    if  ;; label = @1
      loop  ;; label = @2
        local.get 0
        i32.load offset=76
        i32.const 0
        i32.ge_s
        if (result i32)  ;; label = @3
          i32.const 1
        else
          i32.const 0
        end
        drop
        local.get 0
        i32.load offset=20
        local.get 0
        i32.load offset=28
        i32.gt_u
        if  ;; label = @3
          local.get 0
          call 13
          local.get 1
          i32.or
          local.set 1
        end
        local.get 0
        i32.load offset=56
        local.tee 0
        br_if 0 (;@2;)
      end
    end
    local.get 1)
  (func (;13;) (type 0) (param i32) (result i32)
    (local i32 i32)
    block  ;; label = @1
      local.get 0
      i32.load offset=20
      local.get 0
      i32.load offset=28
      i32.le_u
      br_if 0 (;@1;)
      local.get 0
      i32.const 0
      i32.const 0
      local.get 0
      i32.load offset=36
      call_indirect (type 5)
      drop
      local.get 0
      i32.load offset=20
      br_if 0 (;@1;)
      i32.const -1
      return
    end
    local.get 0
    i32.load offset=4
    local.tee 1
    local.get 0
    i32.load offset=8
    local.tee 2
    i32.lt_u
    if  ;; label = @1
      local.get 0
      local.get 1
      local.get 2
      i32.sub
      i64.extend_i32_s
      i32.const 1
      local.get 0
      i32.load offset=40
      call_indirect (type 6)
      drop
    end
    local.get 0
    i32.const 0
    i32.store offset=28
    local.get 0
    i64.const 0
    i64.store offset=16
    local.get 0
    i64.const 0
    i64.store offset=4 align=4
    i32.const 0)
  (func (;14;) (type 0) (param i32) (result i32)
    local.get 0
    memory.grow)
  (table (;0;) 2 2 funcref)
  (memory (;0;) 256 256)
  (global (;0;) (mut i32) (i32.const 5243936))
  (global (;1;) i32 (i32.const 1044))
  (export "memory" (memory 0))
  (export "__indirect_function_table" (table 0))
  (export "add" (func 5))
  (export "_start" (func 6))
  (export "__errno_location" (func 8))
  (export "fflush" (func 12))
  (export "stackSave" (func 9))
  (export "stackRestore" (func 10))
  (export "stackAlloc" (func 11))
  (export "__data_end" (global 1))
  (export "__growWasmMemory" (func 14))
  (elem (;0;) (i32.const 1) func 4))
