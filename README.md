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

## 📦 How to Build and Run

Ensure you have OpenSSL installed:

```bash
sudo apt install libssl-dev
Build Instructions
1️⃣ Build the Server

cd file_transfer_system/server
make
2️⃣ Build the Client
In a new terminal:

cd file_transfer_system/client
make
🚀 Running the Project
🖥️ Step 1: Start the Server
From the server directory:

./server
You should see:

🔊 Server ready...
💻 Step 2: Launch the Client
From the client directory in a new terminal:

./client
You will see:

📦 Command Options: REGISTER / UPLOAD / DOWNLOAD / EXIT
📂 How to Use the Client
✅ Register

Command: REGISTER
Username: your_username
Password: ****
If successful:

✅ Registered.
⬆️ Upload a File
Command: UPLOAD
Username: your_username
Password: ****
File to upload: /path/to/your/file.pdf
public or private? public
If successful:

⬆ Upload done.
⬇️ Download a File

Command: DOWNLOAD
Username: your_username
Password: ****
File to download: uploaded_filename
If authorized:

⬇ Downloaded 1024B as downloaded_uploaded_filename
Integrity: ✅ OK
If unauthorized or private and not owned by the user:

❌ Access denied.
🔍 SHA256 Integrity Verification
Each uploaded file is hashed using OpenSSL’s SHA256 algorithm. The server stores this hash.

When a user downloads a file:

The client recalculates the file's SHA256 hash.

The hash is compared against the server-side value.

If they match, the integrity is verified:

✅ OK → File not altered.

❌ Mismatch → File corrupted or tampered.
