package org.flashvm.example; /* Keep the socket preparation helper in the Android example host package. */
import android.net.LocalSocket; /* Use the actual Android Unix-domain socket client. */
import android.net.LocalSocketAddress; /* Select the real namespace explicitly. */
import java.io.IOException; /* Preserve ordinary native connection and cleanup failures. */
/* Named Unix sockets exist on Android; abstract names are not filesystem paths. */
public final class SocketNames { /* Demonstrate actual Java-side endpoint preparation independently of speech permissions. */
    /* Connect to an explicitly installed abstract service; do not prefix the Java name with a NUL. */
    public static LocalSocket abstractService(String name) throws IOException { /* The caller owns the returned connected socket. */
        LocalSocket socket = new LocalSocket(); /* Create a real Android AF_LOCAL client. */
        try { /* Clean partial state on an actual native connection failure. */
            socket.connect(new LocalSocketAddress(name, LocalSocketAddress.Namespace.ABSTRACT)); /* Android supplies the native leading NUL for this namespace. */
            return socket; /* Transfer a real connected socket, not a fictitious service. */
        } catch (IOException failure) { /* Preserve normal system failure without assertions. */
            try { socket.close(); } catch (IOException cleanup) { failure.addSuppressed(cleanup); } /* Release the native descriptor while preserving both errors. */
            throw failure; /* Let the host report service absence or permission failure. */
        } /* Finish failed connection cleanup. */
    } /* Finish actual abstract endpoint connection. */
    /* Connect to an explicitly prepared filesystem socket path in the application's permitted environment. */
    public static LocalSocket filesystemService(String path) throws IOException { /* Filesystem paths use their own namespace and lifetime. */
        LocalSocket socket = new LocalSocket(); /* Create the actual Android local socket. */
        try { /* Preserve deterministic native handle cleanup. */
            socket.connect(new LocalSocketAddress(path, LocalSocketAddress.Namespace.FILESYSTEM)); /* Use the real path without pretending every Android socket is abstract. */
            return socket; /* The caller closes this actual service connection when finished. */
        } catch (IOException failure) { /* Report real OS path/service failures. */
            try { socket.close(); } catch (IOException cleanup) { failure.addSuppressed(cleanup); } /* Do not leak a failed connection attempt. */
            throw failure; /* Preserve ordinary host error handling. */
        } /* Finish failed filesystem connection cleanup. */
    } /* Finish actual filesystem endpoint connection. */
} /* Finish named Android Unix socket preparation. */
