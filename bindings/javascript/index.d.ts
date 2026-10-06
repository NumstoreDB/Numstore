// Type definitions for the numstore Node.js bindings.

/** Anything numstore can read bytes from or write bytes into. */
export type Bytes = ArrayBufferView | ArrayBuffer | SharedArrayBuffer;
export type Int = number | bigint;

/**
 * Errors thrown by the bindings carry a `code`:
 *  - "ENUMSTORE"       the C library reported an error (message from ns_strerror)
 *  - "ENUMSTORE_STATE" misuse: closed database, finished transaction, freed plan
 */
export interface NumstoreError extends Error {
  code: 'ENUMSTORE' | 'ENUMSTORE_STATE';
}

declare abstract class Store implements Disposable {
  readonly path: string;
  /** True once close() or crash() has been called. */
  readonly closed: boolean;
  /** Begin an explicit transaction. You must commit() or rollback() it. */
  begin(): Transaction;
  /**
   * Run fn inside a transaction: committed on return, rolled back on throw.
   * fn must be synchronous.
   */
  transaction<T>(fn: (tx: Transaction) => T): T;
  /** Close gracefully; frees live plans. Throws if transactions are open. */
  close(): void;
  /** Close abruptly; open transactions roll back on next open. */
  crash(): void;
  [Symbol.dispose](): void;
}

export class Database extends Store {
  constructor(path: string | URL);
  /** A query that takes no data, e.g. "create foo u32". If tx is omitted a one-shot transaction is used. */
  execute(query: string, tx?: Transaction | null): void;
  getVar(query: string, tx?: Transaction | null): Variable;
  /** READ/REMOVE query into dest. */
  read(query: string, dest: Bytes, tx?: Transaction | null): number;
  /** INSERT/WRITE query from src. */
  write(query: string, src: Bytes, tx?: Transaction | null): number;
  /** READ/REMOVE query into a newly allocated Buffer. */
  readAll(query: string, tx?: Transaction | null): Buffer;
  plan(query: string): Plan;
}

export interface StridedOptions {
  /** Starting offset in bytes. Default 0. */
  offset?: Int;
  /** Bytes per element. Default: the buffer's BYTES_PER_ELEMENT, else 1. */
  elementSize?: Int;
  /** Step between elements, in elements. Default 1. */
  stride?: Int;
  /** Number of elements. Default: buffer.byteLength / elementSize. */
  count?: Int;
}

export class SmartFile extends Store {
  constructor(path: string | URL);
  /** Size in bytes, as seen by tx. */
  size(tx?: Transaction | null): number;
  /** Insert src at byte offset, shifting later bytes right. */
  insert(src: Bytes, offset: Int, tx?: Transaction | null): number;
  write(src: Bytes, options?: StridedOptions | null, tx?: Transaction | null): number;
  read(dest: Bytes, options?: StridedOptions | null, tx?: Transaction | null): number;
  /** Remove elements; copies them into dest first if given (count required when dest is null). */
  remove(dest: Bytes | null, options?: StridedOptions | null, tx?: Transaction | null): number;
}

export class Transaction implements Disposable {
  private constructor();
  readonly database: Database | SmartFile;
  readonly active: boolean;
  /** If commit fails the transaction stays active so you can roll back. */
  commit(): void;
  rollback(): void;

  // Database transactions
  execute(query: string): void;
  getVar(query: string): Variable;
  readAll(query: string): Buffer;
  read(query: string, dest: Bytes): number;
  write(query: string, src: Bytes): number;

  // SmartFile transactions
  size(): number;
  insert(src: Bytes, offset: Int): number;
  read(dest: Bytes, options?: StridedOptions | null): number;
  write(src: Bytes, options?: StridedOptions | null): number;
  remove(dest: Bytes | null, options?: StridedOptions | null): number;

  /** Rolls back if still active. */
  [Symbol.dispose](): void;
}

export class Plan implements Disposable {
  private constructor();
  readonly database: Database;
  readonly query: string;
  readonly freed: boolean;
  execute(tx?: Transaction | null): void;
  getVar(tx?: Transaction | null): Variable;
  read(dest: Bytes, tx?: Transaction | null): number;
  write(src: Bytes, tx?: Transaction | null): number;
  readAll(tx?: Transaction | null): Buffer;
  free(): void;
  [Symbol.dispose](): void;
}

export class Variable implements Disposable {
  private constructor();
  /** Length in elements; a bigint if it exceeds 2^53. */
  readonly length: number | bigint;
  readonly freed: boolean;
  free(): void;
  [Symbol.dispose](): void;
}

export function open(path: string | URL): Database;
export function openSmartFile(path: string | URL): SmartFile;
export function cleanup(path: string | URL): void;

export const constants: Readonly<{
  PAGE_SIZE: number;
  END: bigint;
  SMF_END: bigint;
}>;
