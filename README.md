# 🔐 Secure File Transfer System (Terminal-Based, C Language)

This project is a secure and lightweight file transfer system built entirely in C, designed to run from the terminal. It follows a client-server architecture using TCP sockets and offers a full-fledged workflow to register users, securely upload files, and download files with strict access control and integrity verification. 

The main goal of this project is to simulate a real-world secure file-sharing environment, especially under a Unix-like OS, where the server handles incoming file transactions and authentication, and clients interact with the server via commands.

---

## 🛠️ How It Works

When a user launches the client application, they are prompted to enter one of three commands: `REGISTER`, `UPLOAD`, or `DOWNLOAD`. For all operations, the user must provide a valid username and password. On the backend, the server authenticates these credentials using a simple credential store (`users.txt`) that auto-creates on the first run.

For **uploading**, the user is prompted to choose a file path and whether the file should be public or private. Public files are accessible by any registered user, whereas private files can only be downloaded by the uploader. The file is then transmitted to the server using buffered I/O, renamed with a unique user-specific prefix to prevent name collisions, and saved in an `uploads/` directory. The SHA256 hash of the file is computed before the upload and recorded for integrity verification.

During **download**, the server checks if the requested file exists and whether the user has permission to access it. If the file is public or belongs to the requesting user, the download proceeds. Once received, the client again computes the SHA256 hash and compares it with the server-side hash to verify that the file has not been altered in transit.

All uploaded files are tracked via `file_index.txt`, and every upload/download operation is logged into `logs.txt`, which aids in traceability and debugging.

---

## ✨ Features

- **Terminal-only CLI Interface:** Clean, interactive, no-frills UI that runs directly in the Linux terminal. Ideal for demonstrating core system-level functionality.
  
- **Authentication System:** Simple yet effective authentication using a text-based credential store (`users.txt`). Prevents duplicate usernames.

- **Access Control:** During upload, users mark files as public or private. Access to files is then enforced accordingly during downloads.

- **Data Integrity Verification:** All uploaded files are hashed using SHA256 via OpenSSL. The client re-hashes the file upon download to confirm the integrity and detect any tampering.

- **Dynamic File Naming:** Uploaded files are stored using a unique prefix (e.g., user ID) to ensure no two uploads overwrite each other.

- **Logging and Indexing:** Every file operation is logged in `logs.txt` and indexed in `file_index.txt`, auto-managed by the system.

- **Auto-Creation of Required Files:** If `users.txt`, `logs.txt`, or `file_index.txt` do not exist, they are automatically created at runtime.

- **Supports All File Types:** You can upload any file type (text, PDFs, media, etc.) as long as the path is valid.

---
# 🔐 SHA256 Integrity Verification using OpenSSL

This system ensures **data integrity and tamper detection** for every uploaded and downloaded file using **SHA256 cryptographic hashing**, implemented via the OpenSSL library.

---

## 📌 What is SHA256?

SHA256 (Secure Hash Algorithm 256-bit) is a cryptographic hash function that converts data into a fixed 256-bit (64-character hexadecimal) string. Any change in the input — even a single byte — will produce a completely different hash. This makes it ideal for **detecting tampering or corruption**.

---

## 🔍 How It Works in This Project

### 🟡 **Before Upload (Client Side):**
- The file selected for upload is opened and read in chunks.
- Using OpenSSL's hashing functions, the **SHA256 hash is computed**.
- The file is then sent to the server **along with the hash string**.

### 🔵 **On the Server:**
- The server receives the uploaded file and stores it securely.
- It also keeps a record of the **client-computed SHA256 hash** associated with that file.
- This hash is stored in memory or logged in `file_index.txt` for later verification.

### 🟢 **During Download (Client Side):**
1. The user requests a file to download.
2. The server checks access permissions and sends the file **and its original SHA256 hash** to the client.
3. The client:
   - Saves the file locally.
   - **Recomputes the SHA256 hash** of the received file.
   - **Compares it** to the server-provided hash.
4. The result is displayed:
   - ✅ **Integrity: OK** if the hashes match.
   - ❌ **Integrity: MISMATCH** if the file has been altered or corrupted.

---

## 🧪 OpenSSL Functions Used

This implementation uses OpenSSL’s `EVP` interface for digest computation:

```c
EVP_MD_CTX* ctx = EVP_MD_CTX_new();
EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);

while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
    EVP_DigestUpdate(ctx, buf, n);

EVP_DigestFinal_ex(ctx, hash, &len);
EVP_MD_CTX_free(ctx);



