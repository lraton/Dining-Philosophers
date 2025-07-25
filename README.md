# 🍝 Dining Philosophers with Deadlock and Starvation Detection

This C program simulates the classic **Dining Philosophers Problem** using **POSIX semaphores** and **shared memory**, with configurable options for:

- 🔁 Deadlock detection  
- 🕒 Starvation detection  
- ✅ Deadlock-free (safe) strategy  
- 📥 Graceful shutdown on `Ctrl+C`  

---

## 🧠 How It Works

Each philosopher is implemented as a child process that tries to acquire two forks (represented by semaphores) to eat. The following modes can be selected:

- **Deadlock Simulation**: Each philosopher grabs one fork and waits for the other, causing potential deadlock.
- **Deadlock Detection**: An extra process monitors fork ownership via a shared matrix to detect circular waits.
- **Starvation Detection**: Uses `sem_timedwait` with timeout to detect if a philosopher waits too long.
- **Safe Mode**: Philosophers acquire forks in a coordinated way to avoid deadlocks entirely.

---

## 🧾 Usage

### 🔧 Compile

```bash
gcc -o philosophers philosophers.c -lpthread -lrt
