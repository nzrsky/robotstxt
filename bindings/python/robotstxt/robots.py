"""
Low-level ctypes bindings for the robots.txt C API.
"""

import ctypes
import os
import sys
from ctypes import c_bool, c_char_p, c_double, c_int, c_int8, c_size_t, c_void_p, POINTER, Structure
from pathlib import Path
from typing import List, Optional, Tuple


def _find_library() -> str:
    """Find the robots shared library."""
    # Library names by platform
    if sys.platform == "darwin":
        lib_names = ["librobots.dylib", "librobots.1.dylib"]
    elif sys.platform == "win32":
        lib_names = ["robots.dll", "librobots.dll"]
    else:
        lib_names = ["librobots.so", "librobots.so.1"]

    # Search paths
    search_paths = [
        # Relative to this file
        Path(__file__).parent,
        Path(__file__).parent.parent,
        Path(__file__).parent.parent.parent,
        # Build directories
        Path(__file__).parent.parent.parent.parent / "cmake-build",
        Path(__file__).parent.parent.parent.parent / "build",
        # System paths
        Path("/usr/local/lib"),
        Path("/usr/lib"),
    ]

    # Add LD_LIBRARY_PATH / DYLD_LIBRARY_PATH
    env_path = os.environ.get("LD_LIBRARY_PATH", "") or os.environ.get("DYLD_LIBRARY_PATH", "")
    for p in env_path.split(":"):
        if p:
            search_paths.append(Path(p))

    for search_path in search_paths:
        for lib_name in lib_names:
            lib_path = search_path / lib_name
            if lib_path.exists():
                return str(lib_path)

    # Try loading by name (system search)
    for lib_name in lib_names:
        try:
            ctypes.CDLL(lib_name)
            return lib_name
        except OSError:
            pass

    raise OSError(
        f"Could not find robots library. Searched: {lib_names}. "
        "Make sure to build the library first with: cmake -B build && cmake --build build"
    )


# Load the library
_lib = ctypes.CDLL(_find_library())


# =============================================================================
# C API structures
# =============================================================================

class RequestRate(Structure):
    """Request-rate value (requests per time period)."""
    _fields_ = [
        ("requests", c_int),
        ("seconds", c_int),
    ]


class ContentSignal(Structure):
    """Content-Signal values for AI content preferences.

    Each field is a tri-state: -1 = not set, 0 = no, 1 = yes.
    """
    _fields_ = [
        ("ai_train", c_int8),
        ("ai_input", c_int8),
        ("search", c_int8),
    ]


# =============================================================================
# C API function signatures
# =============================================================================

# Matcher lifecycle
_lib.robots_matcher_create.argtypes = []
_lib.robots_matcher_create.restype = c_void_p

_lib.robots_matcher_free.argtypes = [c_void_p]
_lib.robots_matcher_free.restype = None

# URL checking
_lib.robots_allowed_by_robots.argtypes = [
    c_void_p, c_char_p, c_size_t, c_char_p, c_size_t, c_char_p, c_size_t
]
_lib.robots_allowed_by_robots.restype = c_bool

_lib.robots_allowed_by_robots_multi.argtypes = [
    c_void_p, c_char_p, c_size_t,
    POINTER(c_char_p), POINTER(c_size_t), c_size_t,
    c_char_p, c_size_t
]
_lib.robots_allowed_by_robots_multi.restype = c_bool

# Matcher state accessors
_lib.robots_matching_line.argtypes = [c_void_p]
_lib.robots_matching_line.restype = c_int

_lib.robots_ever_seen_specific_agent.argtypes = [c_void_p]
_lib.robots_ever_seen_specific_agent.restype = c_bool

# Crawl-delay
_lib.robots_has_crawl_delay.argtypes = [c_void_p]
_lib.robots_has_crawl_delay.restype = c_bool

_lib.robots_get_crawl_delay.argtypes = [c_void_p]
_lib.robots_get_crawl_delay.restype = c_double

# Request-rate
_lib.robots_has_request_rate.argtypes = [c_void_p]
_lib.robots_has_request_rate.restype = c_bool

_lib.robots_get_request_rate.argtypes = [c_void_p, POINTER(RequestRate)]
_lib.robots_get_request_rate.restype = c_bool

# Content-Signal
_lib.robots_content_signal_supported.argtypes = []
_lib.robots_content_signal_supported.restype = c_bool

_lib.robots_has_content_signal.argtypes = [c_void_p]
_lib.robots_has_content_signal.restype = c_bool

_lib.robots_get_content_signal.argtypes = [c_void_p, POINTER(ContentSignal)]
_lib.robots_get_content_signal.restype = c_bool

_lib.robots_allows_ai_train.argtypes = [c_void_p]
_lib.robots_allows_ai_train.restype = c_bool

_lib.robots_allows_ai_input.argtypes = [c_void_p]
_lib.robots_allows_ai_input.restype = c_bool

_lib.robots_allows_search.argtypes = [c_void_p]
_lib.robots_allows_search.restype = c_bool

# Utility functions
_lib.robots_is_valid_user_agent.argtypes = [c_char_p, c_size_t]
_lib.robots_is_valid_user_agent.restype = c_bool

_lib.robots_version.argtypes = []
_lib.robots_version.restype = c_char_p


# =============================================================================
# Python API
# =============================================================================

def get_version() -> str:
    """Get the library version string."""
    return _lib.robots_version().decode("utf-8")


def is_valid_user_agent(user_agent: str) -> bool:
    """Check if a user-agent string contains only valid characters [a-zA-Z_-]."""
    ua_bytes = user_agent.encode("utf-8")
    return _lib.robots_is_valid_user_agent(ua_bytes, len(ua_bytes))


class RobotsMatcher:
    """
    Robots.txt matcher - checks if URLs are allowed for given user-agents.

    Example:
        matcher = RobotsMatcher()

        robots_txt = '''
        User-agent: *
        Disallow: /admin/
        Crawl-delay: 2
        '''

        if matcher.is_allowed(robots_txt, "Googlebot", "https://example.com/page"):
            print("Access allowed")

        delay = matcher.crawl_delay
        if delay is not None:
            print(f"Crawl delay: {delay}s")
    """

    def __init__(self):
        self._ptr = _lib.robots_matcher_create()
        if not self._ptr:
            raise MemoryError("Failed to create RobotsMatcher")

    def __del__(self):
        if hasattr(self, "_ptr") and self._ptr:
            _lib.robots_matcher_free(self._ptr)
            self._ptr = None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self._ptr:
            _lib.robots_matcher_free(self._ptr)
            self._ptr = None
        return False

    def is_allowed(
        self,
        robots_txt: str,
        user_agent: str,
        url: str,
    ) -> bool:
        """
        Check if a URL is allowed for a single user-agent.

        Args:
            robots_txt: The robots.txt content.
            user_agent: The user-agent string to check.
            url: The URL to check (should be %-encoded per RFC3986).

        Returns:
            True if the URL is allowed, False otherwise.
        """
        robots_bytes = robots_txt.encode("utf-8")
        ua_bytes = user_agent.encode("utf-8")
        url_bytes = url.encode("utf-8")

        return _lib.robots_allowed_by_robots(
            self._ptr,
            robots_bytes, len(robots_bytes),
            ua_bytes, len(ua_bytes),
            url_bytes, len(url_bytes),
        )

    def is_allowed_multi(
        self,
        robots_txt: str,
        user_agents: List[str],
        url: str,
    ) -> bool:
        """
        Check if a URL is allowed for multiple user-agents.
        Rules from all matching user-agents are combined.

        Args:
            robots_txt: The robots.txt content.
            user_agents: List of user-agent strings to check.
            url: The URL to check (should be %-encoded per RFC3986).

        Returns:
            True if the URL is allowed, False otherwise.
        """
        robots_bytes = robots_txt.encode("utf-8")
        url_bytes = url.encode("utf-8")

        # Prepare user-agent arrays
        ua_bytes_list = [ua.encode("utf-8") for ua in user_agents]
        ua_array = (c_char_p * len(user_agents))(*ua_bytes_list)
        ua_lens = (c_size_t * len(user_agents))(*[len(ua) for ua in ua_bytes_list])

        return _lib.robots_allowed_by_robots_multi(
            self._ptr,
            robots_bytes, len(robots_bytes),
            ua_array, ua_lens, len(user_agents),
            url_bytes, len(url_bytes),
        )

    @property
    def matching_line(self) -> int:
        """Get the line number that matched, or 0 if no match."""
        return _lib.robots_matching_line(self._ptr)

    @property
    def ever_seen_specific_agent(self) -> bool:
        """Check if a specific user-agent block was found (not just '*')."""
        return _lib.robots_ever_seen_specific_agent(self._ptr)

    @property
    def crawl_delay(self) -> Optional[float]:
        """Get the crawl-delay in seconds, or None if not specified."""
        if _lib.robots_has_crawl_delay(self._ptr):
            return _lib.robots_get_crawl_delay(self._ptr)
        return None

    @property
    def request_rate(self) -> Optional[Tuple[int, int]]:
        """
        Get the request-rate as (requests, seconds), or None if not specified.

        Example: (1, 10) means 1 request per 10 seconds.
        """
        rate = RequestRate()
        if _lib.robots_get_request_rate(self._ptr, ctypes.byref(rate)):
            return (rate.requests, rate.seconds)
        return None

    @property
    def content_signal(self) -> Optional[dict]:
        """
        Get Content-Signal values, or None if not specified.

        Returns a dict with keys 'ai_train', 'ai_input', 'search'.
        Values are True, False, or None (not set).
        """
        if not _lib.robots_content_signal_supported():
            return None

        signal = ContentSignal()
        if not _lib.robots_get_content_signal(self._ptr, ctypes.byref(signal)):
            return None

        def tri_state(val: int) -> Optional[bool]:
            if val == -1:
                return None
            return val == 1

        return {
            "ai_train": tri_state(signal.ai_train),
            "ai_input": tri_state(signal.ai_input),
            "search": tri_state(signal.search),
        }

    @property
    def allows_ai_train(self) -> bool:
        """Check if AI training is allowed (defaults to True if not specified)."""
        return _lib.robots_allows_ai_train(self._ptr)

    @property
    def allows_ai_input(self) -> bool:
        """Check if AI input is allowed (defaults to True if not specified)."""
        return _lib.robots_allows_ai_input(self._ptr)

    @property
    def allows_search(self) -> bool:
        """Check if search indexing is allowed (defaults to True if not specified)."""
        return _lib.robots_allows_search(self._ptr)
