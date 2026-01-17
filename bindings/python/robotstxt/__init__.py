"""
Python bindings for Google's robots.txt parser library.

Example usage:
    from robotstxt import RobotsMatcher

    matcher = RobotsMatcher()
    robots_txt = '''
    User-agent: *
    Disallow: /admin/
    Allow: /admin/public/
    '''

    allowed = matcher.is_allowed(robots_txt, "Googlebot", "https://example.com/admin/secret")
    print(f"Access: {'allowed' if allowed else 'disallowed'}")
"""

from .robots import RobotsMatcher, is_valid_user_agent, get_version

__all__ = ["RobotsMatcher", "is_valid_user_agent", "get_version"]
__version__ = "1.0.0"
