from typing import Generator

DEFAULT_GRID_WIDTH = 105
DEFAULT_GRID_HEIGHT = 111
DEFAULT_MIDDLE_LINE_WIDTH = 0
MAXIMUM_UNIT_SIZE = 8


def qr_mul_scalar(qr: tuple[int, int], scalar: int) -> tuple[int, int]:
    q, r = qr
    return (q * scalar, r * scalar)


def qr_add_qr(a: tuple[int, int], b: tuple[int, int]) -> tuple[int, int]:
    aq, ar = a
    bq, br = b
    return (aq + bq, ar + br)


def qr_negate(a: tuple[int, int]) -> tuple[int, int]:
    q, r = a
    return (-q, -r)


def qr_reflect_over_middle_line(qr: tuple[int, int]) -> tuple[int, int]:
    q, r = qr
    return (q + r, -r)


class GridConfig:
    def __init__(self, width: int, height: int, middle_line_width: int):
        self.width_ = int(width)
        self.height_ = int(height)
        self.middle_line_width_ = int(middle_line_width)
        self.rect_width_extent_ = (self.width_ - 1) // 2
        self.rect_height_extent_ = (self.height_ - 1) // 2
        self.rotations_ = [
            (1, 0),  # right
            (0, 1),  # bottom right
            (-1, 1),  # bottom left
            (-1, 0),  # left
            (0, -1),  # top left
            (1, -1),  # top right
        ]

    @property
    def rect_width_extent(self) -> int:
        return self.rect_width_extent_

    @property
    def rect_height_extent(self) -> int:
        return self.rect_height_extent_

    @property
    def middle_line_width(self) -> int:
        return self.middle_line_width_

    def rect_q_limit_min(self, r: int) -> int:
        left: int = -self.rect_width_extent
        left_offset: int = r >> 1
        return left - left_offset

    def rect_q_limit_max(self, r: int) -> int:
        right: int = self.rect_width_extent
        right_offset: int = r >> 1
        return right - right_offset

    def rect_r_limit_min(self) -> int:
        return -self.rect_height_extent

    def rect_r_limit_max(self) -> int:
        return self.rect_height_extent

    def group_offset_to_corner(self, group_extent: int, direction: int) -> tuple[int, int]:
        return qr_mul_scalar(self.rotations_[direction], group_extent)

    def group_offset_to_neighbor_group(self, group_extent: int, direction: int) -> tuple[int, int]:
        to_src_group_corner = self.group_offset_to_corner(group_extent, direction)
        to_dst_group_corner = qr_add_qr(to_src_group_corner, self.rotations_[direction])
        to_dst_group_center = qr_add_qr(
            to_dst_group_corner, qr_negate(self.group_offset_to_corner(group_extent, (direction + 4) % 6))
        )
        return to_dst_group_center

    def is_valid_position(self, position: tuple[int, int]) -> bool:
        q, r = position
        return (
            r >= self.rect_r_limit_min()
            and r <= self.rect_r_limit_max()
            and q >= self.rect_q_limit_min(r)
            and q <= self.rect_q_limit_max(r)
        )

    def is_valid_hex_group(self, center: tuple[int, int], size: int) -> bool:
        return all(self.is_valid_position(qr_add_qr(center, qr_mul_scalar(offset, size))) for offset in self.rotations_)

    def fill_red_side(self, unit_size: int) -> Generator[tuple[int, int], None, None]:
        for blue_position in self.fill_blue_side(unit_size):
            yield qr_reflect_over_middle_line(blue_position)

    def fill_blue_side(self, unit_size: int) -> Generator[tuple[int, int], None, None]:
        bottom_right = self.group_offset_to_neighbor_group(unit_size, 0)
        bottom_left = self.group_offset_to_neighbor_group(unit_size, 2)
        top_right = self.group_offset_to_neighbor_group(unit_size, 5)

        added_anything = True

        def walk_top_right_diag(start_position: tuple[int, int]) -> Generator[tuple[int, int], None, None]:
            nonlocal added_anything
            position = qr_add_qr(start_position, top_right)
            while position[1] - unit_size > 0 and position[0] + unit_size <= self.rect_q_limit_max(position[1]):
                if self.is_valid_hex_group(position, unit_size):
                    added_anything = True
                    yield position
                position = qr_add_qr(position, top_right)

        def walk_bottom_left_diag(start_position: tuple[int, int]) -> Generator[tuple[int, int], None, None]:
            nonlocal added_anything
            position = qr_add_qr(start_position, bottom_left)
            while position[1] + unit_size <= self.rect_height_extent and position[
                0
            ] - unit_size >= self.rect_q_limit_min(position[1]):
                if self.is_valid_hex_group(position, unit_size):
                    added_anything = True
                    yield position
                position = qr_add_qr(position, bottom_left)

        start_r = 1 + self.middle_line_width + unit_size
        start_q = self.rect_q_limit_min(start_r) + unit_size
        bottom_right_diag = (start_q, start_r)
        while added_anything:
            added_anything = False

            if self.is_valid_hex_group(bottom_right_diag, unit_size):
                added_anything = True
                yield bottom_right_diag

            yield from walk_top_right_diag(bottom_right_diag)
            yield from walk_bottom_left_diag(bottom_right_diag)

            bottom_right_diag = qr_add_qr(bottom_right_diag, bottom_right)
